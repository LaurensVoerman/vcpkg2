#include "FFmpegImageStream.hpp"
#include "FFmpegAudioStream.hpp"
#include "FFmpegParameters.hpp"
#if defined(USE_OPENAL)
#include "AudioSink.h"
#endif
//#include "Logger.h"

#include <OpenThreads/ScopedLock>
#include <osg/Notify>
#include <osg/Version>

#include <memory>
#include "Config.h"

#define STREAM_TIMEOUT_IN_SECONDS_TO_CONSIDER_IT_DEAD   10


namespace osgFFmpeg {



FFmpegImageStream::FFmpegImageStream() :
    m_decoder(0),
    m_commands(0),
    m_framepublishBarier(2),
    _lastUpdateTS(0.)
{
    setOrigin(osg::Image::TOP_LEFT);

    std::unique_ptr<FFmpegDecoder> decoder(new FFmpegDecoder);
    std::unique_ptr<CommandQueue> commands(new CommandQueue);

    m_decoder = decoder.release();
    m_commands = commands.release();
}



FFmpegImageStream::FFmpegImageStream(const FFmpegImageStream & image, const osg::CopyOp & copyop) :
    osg::ImageStream(image, copyop)
{
    // TODO: probably incorrect or incomplete
}



FFmpegImageStream::~FFmpegImageStream()
{
    OSG_INFO<<"Destructing FFmpegImageStream..."<<std::endl;

    quit(true);

    OSG_INFO<<"Have done quit"<<std::endl;

    // release athe audio streams to make sure that the decoder doesn't retain any external
    // refences.
    getAudioStreams().clear();

    // destroy the decoder and associated threads
    m_decoder = 0;


    delete m_commands;

    OSG_INFO<<"Destructed FFMpegImageStream."<<std::endl;
}



bool FFmpegImageStream::open(const std::string & filename, FFmpegParameters* parameters)
{
    setFileName(filename);

    if (! m_decoder->open(filename, parameters))
        return false;

    // do not wait for slaves publishing frames when we have a internet stream
    if(filename.find("rtsp://") != std::string::npos){
	m_framepublishBarier.invalidate();
    }

    setImage(
        m_decoder->video_decoder().width(), m_decoder->video_decoder().height(), 1, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE,
        const_cast<unsigned char *>(m_decoder->video_decoder().image()), NO_DELETE
    );


    setPixelAspectRatio(m_decoder->video_decoder().pixelAspectRatio());

    OSG_NOTICE<<"ffmpeg::open("<<filename<<") size("<<s()<<", "<<t()<<") aspect ratio "<<m_decoder->video_decoder().pixelAspectRatio()<<std::endl;

#if 1
    // swscale is reported errors and then crashing when rescaling video of size less than 10 by 10.
    if (s()<=10 || t()<=10) return false;
#endif

    m_decoder->video_decoder().setUserData(this);
    m_decoder->video_decoder().setPublishCallback(publishNewFrame);

    if (m_decoder->audio_decoder().validContext())
    {
        OSG_NOTICE<<"Attaching FFmpegAudioStream"<<std::endl;
        osgFFmpeg::FFmpegAudioStream *ffmpegAudioStream = new FFmpegAudioStream(m_decoder.get());
        getAudioStreams().push_back(ffmpegAudioStream);
#if defined(USE_OPENAL)
        osgRCAudio::AudioSink *sink = NULL;
        if(ffmpegAudioStream->getAudioSink() == NULL) {
            try {
                sink = new osgRCAudio::AudioSink(ffmpegAudioStream);
            } catch(...)
            {
                osgRC::logWARNING("new AudioSink failed");
            }
            ffmpegAudioStream->setAudioSink(sink);
        }
#endif // OPENAL
    }

    _status = PAUSED;
    applyLoopingMode();
    m_decoder->audio_decoder().start();
    m_decoder->video_decoder().start();
    start(); // start thread

    return true;
}



void FFmpegImageStream::play()
{
    m_commands->push(CMD_PLAY);
}



void FFmpegImageStream::pause()
{
    m_commands->push(CMD_PAUSE);
}



void FFmpegImageStream::rewind()
{
    m_commands->push(CMD_REWIND);
}

void FFmpegImageStream::seek(double time) {
    m_seek_time = time;
    m_commands->push(CMD_SEEK);
}



void FFmpegImageStream::quit(bool waitForThreadToExit)
{
    // Stop the packet producer thread
    if (isRunning())
    {
        m_commands->push(CMD_STOP);

        if (waitForThreadToExit)
            join();
    }

    m_framepublishBarier.invalidate();
    // Close the decoder (i.e. flush the decoder packet queues)
    m_decoder->close(waitForThreadToExit);
}

void FFmpegImageStream::setVolume(float volume)
{
    m_decoder->audio_decoder().setVolume(volume);
}

float FFmpegImageStream::getVolume() const
{
    return m_decoder->audio_decoder().getVolume();
}

double FFmpegImageStream::getCreationTime() const
{
    return m_decoder->creation_time();
}

double FFmpegImageStream::getLength() const
{
    return m_decoder->duration();
}


double FFmpegImageStream::getReferenceTime () const
{
    return m_decoder->reference();
}

double FFmpegImageStream::getCurrentTime() const
{
    return m_decoder->reference();
}



double FFmpegImageStream::getFrameRate() const
{
    return m_decoder->video_decoder().frameRate();
}



bool FFmpegImageStream::isImageTranslucent() const
{
    return m_decoder->video_decoder().alphaChannel();
}



void FFmpegImageStream::run()
{
    try
    {
        bool done = false;

        while (! done)
        {
            if (_status == PLAYING)
            {
                bool no_cmd;
                const Command cmd = m_commands->timedPop(no_cmd, 1);

                if (no_cmd)
                {
                    m_decoder->readNextPacket();
                }
                else
                    done = ! handleCommand(cmd);

                // Security check to detect (and stop) dead streams
				// Pjotr: this is done in osg but we don't check yet because of underflow in delta_s this can trigger
                //
#if OSG_VERSION_GREATER_OR_EQUAL(3,4,1)
                if ( _lastUpdateTS > 0. &&
                    osg::Timer::instance()->delta_s(_lastUpdateTS, osg::Timer::instance()->tick()) > STREAM_TIMEOUT_IN_SECONDS_TO_CONSIDER_IT_DEAD )
                {
                    _status = INVALID;
                    done = true;
                }
#endif
            }
            else
            {
                done = ! handleCommand(m_commands->pop());
            }
        }
    }

    catch (const std::exception & error)
    {
        OSG_WARN << "FFmpegImageStream::run : " << error.what() << std::endl;
    }

    catch (...)
    {
        OSG_WARN << "FFmpegImageStream::run : unhandled exception" << std::endl;
    }

    OSG_NOTICE<<"Finished FFmpegImageStream::run()"<<std::endl;
}



void FFmpegImageStream::applyLoopingMode()
{
    m_decoder->loop(getLoopingMode() == LOOPING);
}



bool FFmpegImageStream::handleCommand(const Command cmd)
{
    switch (cmd)
    {
    case CMD_PLAY:
        cmdPlay();
        return true;

    case CMD_PAUSE:
        cmdPause();
        return true;

    case CMD_REWIND:
        cmdRewind();
        return true;

    case CMD_SEEK:
        cmdSeek(m_seek_time);
        return true;

    case CMD_STOP:
        return false;

    default:
        assert(false);
        return false;
    }
}



void FFmpegImageStream::cmdPlay()
{
    if (_status == PAUSED)
    {
        m_decoder->video_decoder().pause(false);
        m_decoder->audio_decoder().pause(false);
    }

    _status = PLAYING;
}



void FFmpegImageStream::cmdPause()
{
    if (_status == PLAYING)
    {
        m_decoder->video_decoder().pause(true);
        m_decoder->audio_decoder().pause(true);
    }

    _status = PAUSED;
}



void FFmpegImageStream::cmdRewind()
{
    m_decoder->rewind();
}

void FFmpegImageStream::cmdSeek(double time)
{
    m_decoder->seek(time);
}


void FFmpegImageStream::publishNewFrame(const FFmpegDecoderVideo &, void * user_data)
{
    FFmpegImageStream * const this_ = reinterpret_cast<FFmpegImageStream*>(user_data);
#if INCLUDE_OSGRC_CONFIG_H(ENABLE_SWAP_BARRIER)
    // wait till we may publish a new frame
    this_->m_framepublishBarier.block();
#endif
    this_->setImage(
        this_->m_decoder->video_decoder().width(), this_->m_decoder->video_decoder().height(), 1, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE,
        const_cast<unsigned char *>(this_->m_decoder->video_decoder().image()), NO_DELETE
    );
#if INCLUDE_OSGRC_CONFIG_H(ENABLE_SWAP_BARRIER)
    // wait till we may decode a new frame
//    this_->m_framepublishBarier.block();
#endif
    // Store the timestamp of this update. Needed to check dead streams
    this_->_lastUpdateTS = osg::Timer::instance()->tick();

}


void FFmpegImageStream::waitforFrameBeginPublish() {
    m_framepublishBarier.block();
}

void FFmpegImageStream::waitforFrameEndPublish() {
//    m_framepublishBarier.block();
}

} // namespace osgFFmpeg
