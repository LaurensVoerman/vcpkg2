#ifndef OSGAUDIO_AUDIOSINK_H
#define OSGAUDIO_AUDIOSINK_H

//Make an implementation for osg::AudioSink
#define OV_EXCLUDE_STATIC_CALLBACKS
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
#endif
#include <osgAudio/SoundManager.h> //inc SoundState inc Stream
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#include <osg/AudioStream>
#include <osg/observer_ptr>
namespace osgRCAudio 
{
    class AudioSink : public osg::AudioSink
    {
    public:
		AudioSink(osg::AudioStream* audioStream) : _started(false), _paused(false), _audioStream(audioStream)
		{
			if (!osgAudio::SoundManager::instance()->initialized()) {
				OSG_WARN << "osgAudio::AudioSink()::play() can't play, soundmanager not initialized!" << std::endl;
				throw std::runtime_error("soundmanager not initialized");
			}
			play();
		}
        ~AudioSink();
        virtual const char * libraryName() const { return "osgAudio"; }
        virtual void play();
        virtual void pause();
        virtual void stop();
        virtual bool playing() const { return _started && !_paused; }
    protected:
        bool                                _started;
        bool                                _paused;
        osg::observer_ptr<osg::AudioStream> _audioStream;
        osg::ref_ptr<osgAudio::SoundState>  _musicState;
    };
}
#ifdef ENABLE_SUBSYSTEM_OPENAL
//the osg::audioStream in the sink needs to update:
class osgAudioStreamUpdater : public openalpp::StreamUpdater 
{
protected:
    const unsigned int buffersize_; // Size of the buffer in bytes
    ALshort *buffer_;
    osg::observer_ptr<osg::AudioStream> _audioStream;
public:
    osgAudioStreamUpdater( osg::AudioStream* _audioStream, const ALuint buffer1,ALuint buffer2, ALenum format,unsigned int frequency, unsigned int buffersize) : 
        StreamUpdater(buffer1,buffer2,format,frequency), buffersize_(buffersize), _audioStream(_audioStream) {
    setCancelModeAsynchronous();
    buffer_ = new ALshort[buffersize_/sizeof(ALshort)];
    }

    void run()
    {
        runmutex_.lock();
        while(!shouldStop() && _audioStream.valid())
        {
            runmutex_.unlock();
            _audioStream->consumeAudioBuffer(buffer_, buffersize_);//make ffmpeg stuff data in *buffer_
            update(buffer_, buffersize_);//pass data to openAL
            runmutex_.lock();
        }
        runmutex_.unlock();
    }

protected:
    virtual ~osgAudioStreamUpdater() {
        stop();
        join();
        openalpp::StreamUpdater::update(buffer_, 0); // call update to do any remaining buffer deallocation
        delete buffer_;
    }
};

class osgAudioStreamConvertUpdater : public osgAudioStreamUpdater {
public:
    osgAudioStreamConvertUpdater( osg::AudioStream* _audioStream, const ALuint buffer1,ALuint buffer2, ALenum format,unsigned int frequency, unsigned int buffersize) : 
    osgAudioStreamUpdater(_audioStream,  buffer1, buffer2,  format, frequency, buffersize) {}
    void run()
    {
        runmutex_.lock();
        while(!shouldStop() && _audioStream.valid())
        {
            runmutex_.unlock();
            _audioStream->consumeAudioBuffer(buffer_, buffersize_);//make ffmpeg stuff data in *buffer_
            char *buffer_as_char = (char *)buffer_;
            if (_audioStream->audioSampleFormat() == osg::AudioStream::SAMPLE_FORMAT_U8) {//output sample size 2(stereo)* 1 byte
                unsigned int nbc = _audioStream->audioNbChannels();//input sample size
                unsigned int sample_count = buffersize_ / nbc;
                if (nbc > 2) {
                    for (unsigned int i=1; i<sample_count;i +=1) {
                        buffer_as_char[(i*2)] = buffer_as_char[i*nbc];
                        buffer_as_char[(i*2)+1] = buffer_as_char[(i*nbc)+1];
                    }
                    sample_count *= 2;//data size in bytes
                }
                update(buffer_, sample_count);//pass data to openAL
            }
            if (_audioStream->audioSampleFormat() == osg::AudioStream::SAMPLE_FORMAT_S16) {//output sample size 2(stereo)* 2 bytes
                unsigned int nbc = 2 * _audioStream->audioNbChannels();//input sample size
                unsigned int sample_count = buffersize_ / nbc;
                if (nbc > 4) {
                    for (unsigned int i=1; i<sample_count;i += 1) {
                        buffer_as_char[(i*4)] = buffer_as_char[i*nbc];
                        buffer_as_char[(i*4)+1] = buffer_as_char[(i*nbc)+1];
                        buffer_as_char[(i*4)+2] = buffer_as_char[(i*nbc)+2];
                        buffer_as_char[(i*4)+3] = buffer_as_char[(i*nbc)+3];
                    }
                    sample_count *= 4;//data size in bytes
                }
                update(buffer_, sample_count);//pass data to openAL
            }
            if (_audioStream->audioSampleFormat() == osg::AudioStream::SAMPLE_FORMAT_S24) {//output sample size 2 bytes (mono) or 4 bytes (stereo)
                unsigned int nbc = 3 * _audioStream->audioNbChannels();//input sample size
                unsigned int sample_count = buffersize_ / nbc;
                if (_audioStream->audioNbChannels() == 1) { //mono
                    for (unsigned int i=0; i<sample_count;i += 1) {
                        buffer_as_char[(i*2)]   = buffer_as_char[(i*nbc)+1];
                        buffer_as_char[(i*2)+1] = buffer_as_char[(i*nbc)+2];
                    }
                    sample_count *= 2;//data size in bytes
                } else {
                    for (unsigned int i=0; i<sample_count;i += 1) {
                        buffer_as_char[(i*4)]   = buffer_as_char[(i*nbc)+1];
                        buffer_as_char[(i*4)+1] = buffer_as_char[(i*nbc)+2];
                        buffer_as_char[(i*4)+2] = buffer_as_char[(i*nbc)+4];
                        buffer_as_char[(i*4)+3] = buffer_as_char[(i*nbc)+5];
                    }
                    sample_count *= 4;//data size in bytes
                }
                update(buffer_, sample_count);//pass data to openAL
            }
            if (_audioStream->audioSampleFormat() == osg::AudioStream::SAMPLE_FORMAT_S32) {//output sample size 2 bytes (mono) or 4 bytes (stereo)
                unsigned int nbc = 4 * _audioStream->audioNbChannels();//input sample size
                unsigned int sample_count = buffersize_ / nbc;
                if (_audioStream->audioNbChannels() == 1) { //mono
                    for (unsigned int i=0; i<sample_count;i += 1) {
                        buffer_as_char[(i*2)]   = buffer_as_char[(i*nbc)+2];
                        buffer_as_char[(i*2)+1] = buffer_as_char[(i*nbc)+3];
                    }
                    sample_count *= 2;//data size in bytes
                } else {
                    for (unsigned int i=0; i<sample_count;i += 1) {
                        buffer_as_char[(i*4)]   = buffer_as_char[(i*nbc)+2];
                        buffer_as_char[(i*4)+1] = buffer_as_char[(i*nbc)+3];
                        buffer_as_char[(i*4)+2] = buffer_as_char[(i*nbc)+6];
                        buffer_as_char[(i*4)+3] = buffer_as_char[(i*nbc)+7];
                    }
                    sample_count *= 4;//data size in bytes
                }
                update(buffer_, sample_count);//pass data to openAL
            }
            if (_audioStream->audioSampleFormat() == osg::AudioStream::SAMPLE_FORMAT_F32) {//output sample size 2 bytes (mono) or 4 bytes (stereo)
                unsigned int nbc = 4 * _audioStream->audioNbChannels();//input sample size
                unsigned int sample_count = buffersize_ / nbc;
                if (_audioStream->audioNbChannels() == 1) { //mono
                    for (unsigned int i=0; i<sample_count;i += 1) {
                        float sample = *((float *)(&buffer_as_char[i*nbc]));
                        sample *= 0x7FFF; //assume float values in -1.0 to 1.0 range
                        ALshort val = (ALshort)sample;
                        buffer_[i]   = val;
                    }
                    sample_count *= 2;//data size in bytes
                } else {
                    for (unsigned int i=0; i<sample_count;i += 1) {
                        float sample = *((float *)(&buffer_as_char[i*nbc]));
                        sample *= 0x7FFF; //assume float values in -1.0 to 1.0 range
                        ALshort val = (ALshort)sample;
                        buffer_[(2*i)]   = val;
                        sample = *((float *)(&buffer_as_char[(i*nbc)+4]));
                        sample *= 0x7FFF; //assume float values in -1.0 to 1.0 range
                        val = (ALshort)sample;
                        buffer_[(2*i)+1]   = val;
                    }
                    sample_count *= 4;//data size in bytes
                }
                update(buffer_, sample_count);//pass data to openAL
            }
            runmutex_.lock();
        }
        runmutex_.unlock();
    }
};
//to change the updater_ I need a modified openalpp::stream
class openalppOsgAudioStream : public openalpp::Stream
{
public:
    openalppOsgAudioStream(osg::AudioStream* audioStream);
};
//this is not a osgAudio::FileStream, (ask ffmpeg for more data instead of reading from file)
//needs an updater to call ffmpeg
class osgAudioOsgAudioStream : public osgAudio::Stream 
{
public:
    osgAudioOsgAudioStream(osg::AudioStream* audioStream) /* throw (openalpp::NameError) */: osgAudio::Stream(0) {
        try {
            _openalppStream = new openalppOsgAudioStream(audioStream);
        }
        catch(openalpp::NameError const& error) { throw openalpp::NameError(error.what()); }
    } // Stream::Stream
};

openalppOsgAudioStream::openalppOsgAudioStream(osg::AudioStream* audioStream) 
{ 
    osg::AudioStream* _audioStream = audioStream;
    OSG_NOTICE<<"  audioFrequency()="<<_audioStream->audioFrequency()<<std::endl;
    OSG_NOTICE<<"  audioNbChannels()="<<_audioStream->audioNbChannels()<<std::endl;
    OSG_NOTICE<<"  audioSampleFormat()="<<_audioStream->audioSampleFormat()<<std::endl;
    unsigned long    ulFrequency =  _audioStream->audioFrequency();
    unsigned long    ulChannels=_audioStream->audioNbChannels();
    unsigned long    ulFormat = 0;
    unsigned long    ulBufferSize;
    unsigned long    bytesPerSample = 0;
    bool convert = false;
    if (_audioStream->audioSampleFormat() == osg::AudioStream::SAMPLE_FORMAT_U8){
        ulFormat = AL_FORMAT_MONO8;
        bytesPerSample = 1;
    }
    if (_audioStream->audioSampleFormat() == osg::AudioStream::SAMPLE_FORMAT_S16){
        ulFormat = AL_FORMAT_MONO16;
        bytesPerSample = 2;
    }
    if (_audioStream->audioSampleFormat() == osg::AudioStream::SAMPLE_FORMAT_S24){
        ulFormat = AL_FORMAT_MONO16;
        bytesPerSample = 3;
        convert = true;
    }
    if (_audioStream->audioSampleFormat() == osg::AudioStream::SAMPLE_FORMAT_S32){
        ulFormat = AL_FORMAT_MONO16;
        bytesPerSample = 4;
        convert = true;
    }
    if (_audioStream->audioSampleFormat() == osg::AudioStream::SAMPLE_FORMAT_F32){
        ulFormat = AL_FORMAT_MONO16;
        bytesPerSample = 4;
        convert = true;
    }
    if (ulChannels > 1) {
        if (ulFormat == AL_FORMAT_MONO8) ulFormat = AL_FORMAT_STEREO8;
        if (ulFormat == AL_FORMAT_MONO16) ulFormat = AL_FORMAT_STEREO16;
        bytesPerSample *= ulChannels;
        if (ulChannels > 2) convert = true;
    }
    if (ulFormat == 0) {
        OSG_NOTICE<<"  audioNbChannels / audioSampleFormat combo not supported."<<std::endl;
        return;
    }
    // Set BufferSize to 250ms (Frequency * bytesPerSample divided by 4 (quarter of a second))
    ulBufferSize = (ulFrequency * bytesPerSample) >> 2;
    // IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
    ulBufferSize -= (ulBufferSize % 2);
    ulBufferSize = ulBufferSize*2;      //sizeof alshort

    if (convert) {
        updater_ = new osgAudioStreamConvertUpdater(audioStream,buffer_->getName(),buffer2_->getAlBuffer(), ulFormat,ulFrequency, ulBufferSize);
    } else {
        updater_ = new osgAudioStreamUpdater(audioStream,buffer_->getName(),buffer2_->getAlBuffer(), ulFormat,ulFrequency, ulBufferSize);
    }
}
#endif
#ifdef ENABLE_SUBSYSTEM_FMOD
#pragma message("audioSink for FMOD nyi!")
#endif


osgRCAudio::AudioSink::~AudioSink() {
    stop();
}

void osgRCAudio::AudioSink::play()
{
    // Depends on --noAudio
    if (!osgAudio::SoundManager::instance()->initialized()) {
        OSG_WARN<<"osgAudio::AudioSink()::play() can't play, soundmanager not initialized!"<<std::endl;
        return;
    }
    if (_started)
    {
        if (_paused)
        {
                _musicState->setPlay( true );
                _paused = false;
        }
        return;
    }

    _started = true;
    _paused = false;
    OSG_NOTICE<<"osgAudio::AudioSink()::play()"<<std::endl;

	_musicState = new osgAudio::SoundState("AudioSink");
    _musicState->allocateSource( 5 );

    osgAudioOsgAudioStream *maStream = new osgAudioOsgAudioStream(_audioStream.get());
    _musicState->setStream( maStream );
    _musicState->setAmbient( true );
//    _musicState->setLooping( true );  //loop is done by ffmpeg
    osgAudio::SoundManager::instance()->addSoundState( _musicState.get() );
    _musicState->setPlay( true );
}

void osgRCAudio::AudioSink::pause()
{
    if (_started)
    {
        _musicState->setPlay( false );
        _paused = true;
    }
}

void osgRCAudio::AudioSink::stop()
{
    if (_started)
    {
        osgAudio::SoundManager::instance()->removeSoundState( _musicState.get() );
        _musicState = NULL; //destruct _musicState
    }
}

#endif
