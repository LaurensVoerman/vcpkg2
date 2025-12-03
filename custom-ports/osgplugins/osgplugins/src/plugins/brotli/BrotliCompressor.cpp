/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2010 Robert Osfield
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/
// Written by Wang Rui, (C) 2010

#include <osg/Notify>
#include <osgDB/Registry>
#include <osgDB/ObjectWrapper>
#include <sstream>

using namespace osgDB;

#define USE_BROTLI
#ifdef USE_BROTLI
#include <brotli/decode.h>
#include <brotli/encode.h>
// #pragma comment (lib, "brotlienc.lib")
// #pragma comment (lib, "brotlidec.lib")
// #pragma comment (lib, "brotlicommon.lib")
// Brotli compressor
class BrotliCompressor : public BaseCompressor
{
public:
    BrotliCompressor() {}

#define BCHUNK 1 << 16

    virtual bool compress(std::ostream& fout, const std::string& src)
    {
        unsigned char out[BCHUNK];
        size_t available_in = src.size();
        const uint8_t* next_in = (uint8_t*)(&(*src.begin()));
        BrotliEncoderState* s = BrotliEncoderCreateInstance(NULL, NULL, NULL);
        if (!s)
        {
            OSG_NOTICE << "BrotliEncoderCreateInstance failed." << std::endl;
            return false;
        }
        BrotliEncoderSetParameter(s, BROTLI_PARAM_QUALITY, 11);
        {
            /* 0, or not specified by user; could be chosen by compressor. */
            uint32_t lgwin = 24;// DEFAULT_LGWIN;
            size_t input_file_length = src.length();
            /* Use file size to limit lgwin. */
            if (input_file_length >= 0) 
            {
                int32_t size = 1 << BROTLI_MIN_WINDOW_BITS;
                lgwin = BROTLI_MIN_WINDOW_BITS;
                while (size < input_file_length) 
                {
                    size <<= 1;
                    lgwin++;
                    if (lgwin == BROTLI_MAX_WINDOW_BITS) break;
                }
            }
            BrotliEncoderSetParameter(s, BROTLI_PARAM_LGWIN, lgwin);
            uint32_t size_hint = input_file_length < (1 << 30) ? input_file_length : (1u << 30);
            BrotliEncoderSetParameter(s, BROTLI_PARAM_SIZE_HINT, size_hint);
        }

        while ((available_in != 0) || BrotliEncoderHasMoreOutput(s))
        {
            size_t available_out = BCHUNK;
            uint8_t* next_out = out;
            if (!BrotliEncoderCompressStream(s, BROTLI_OPERATION_FINISH, &available_in, &next_in, &available_out, &next_out, NULL))
            {
                OSG_NOTICE << "BrotliEncoderCompressStream failed." << std::endl;
                return false;
            }
            size_t have = (BCHUNK) - available_out;
            if (have>0) fout.write((const char*)out, have);

            if (fout.fail())
            {
                BrotliEncoderDestroyInstance(s);
                return false;
            }
        }
        BrotliEncoderDestroyInstance(s);
        return true;
    }
    virtual bool decompress(std::istream& fin, std::string& target)
    {
        int ret = BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT;
        unsigned char in[BCHUNK];
        unsigned char out[BCHUNK];
        BrotliDecoderState* s = BrotliDecoderCreateInstance(NULL, NULL, NULL);
        if (!s)
        {
            OSG_NOTICE << "BrotliDecoderCreateInstance failed." << std::endl;
            return false;
        }
        BrotliDecoderSetParameter(s, BROTLI_DECODER_PARAM_LARGE_WINDOW, 1u);
        do
        {
            fin.read((char *)in, BCHUNK);
            size_t available_in = fin.gcount();
            if (available_in == 0) break;
            const uint8_t* next_in = in;
            do
            {
                size_t avail_out = BCHUNK;
                uint8_t* next_out = out;
                ret = BrotliDecoderDecompressStream(s, &available_in, &next_in, &avail_out, &next_out, 0);
                if (ret == BROTLI_DECODER_RESULT_ERROR)
                {
                    OSG_NOTICE << "BrotliDecoderDecompressStream error." << std::endl;
                    BrotliDecoderDestroyInstance(s);
                    return false;
                }
                size_t have = (BCHUNK) - avail_out;
                target.append((char*)out, have);
            } while (ret == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT);

            /* done when inflate() says it's done */
        } while (ret != BROTLI_DECODER_RESULT_SUCCESS);//BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT

        /* clean up and return */
        BrotliDecoderDestroyInstance(s);
        return (ret == BROTLI_DECODER_RESULT_SUCCESS);
    }
};

REGISTER_COMPRESSOR("brotli", BrotliCompressor)



#endif