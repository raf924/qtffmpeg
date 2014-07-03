#include "videothread.h"
#include "widget.h"
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <mutex>
#include <memory>
#include <thread>
#include <QApplication>
#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif
extern "C"{
#ifdef __cplusplus
#ifdef _STDINT_H
#undef _STDINT_H
#endif
# include <stdint.h>
#endif
#define __STDC_CONSTANT_MACROS
#include <errno.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

class Packet {
public:
    explicit Packet(AVFormatContext* ctxt = nullptr) {
        av_init_packet(&packet);
        packet.data = nullptr;
        if (ctxt) reset(ctxt);
    }

    /*Packet(Packet &&other) : packet(std::move(other.packet)) {
        other.packet.data = NULL;
    }*/

    ~Packet() {
        if (packet.data)
            av_free_packet(&packet);
    }

    void reset(AVFormatContext* ctxt) {
        if (packet.data)
            av_free_packet(&packet);
        if (av_read_frame(ctxt, &packet) < 0)
            packet.data = nullptr;
    }

    AVPacket packet;
};

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    /*Widget * w = new Widget(argv[1]);
    VideoThread * v = new VideoThread(w);
    v->connect(v,SIGNAL(frameReady()),w,SLOT(repaint()));
    v->start();
    w->show();*/
    AVStream * stream = NULL;
    static std::once_flag initFlag;
    std::call_once(initFlag, []() { av_register_all(); });std::shared_ptr<AVFormatContext> avFormat(avformat_alloc_context(), &avformat_free_context);auto avFormatPtr = avFormat.get();
    if (avformat_open_input(&avFormatPtr, argv[1], nullptr, nullptr) != 0)
        throw std::runtime_error("Error while calling avformat_open_input (probably invalid file format)");

    if (avformat_find_stream_info(avFormat.get(), nullptr) < 0)
        throw std::runtime_error("Error while calling avformat_find_stream_info");
    for (unsigned int i = 0; i < avFormat->nb_streams; ++i) {
        stream = avFormat->streams[i];		// pointer to a structure describing the stream
        auto codecType = stream->codec->codec_type;	// the type of data in this stream, notable values are AVMEDIA_TYPE_VIDEO and AVMEDIA_TYPE_AUDIO
        auto codecID = stream->codec->codec_id;		// identifier for the codec
    }
    AVStream* videoStream = nullptr;
    for (auto i = 0; i < avFormat->nb_streams; ++i) {
        if (avFormat->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            // we've found a video stream!
            videoStream = avFormat->streams[i];
            break;
        }
    }

    if (!videoStream)
        throw std::runtime_error("Didn't find any video stream in the file (probably audio file)");// getting the required codec structure
    const auto codec = avcodec_find_decoder(videoStream->codec->codec_id);
    if (codec == nullptr)
        throw std::runtime_error("Codec required by video file not available");

    // allocating a structure
    std::shared_ptr<AVCodecContext> avVideoCodec(avcodec_alloc_context3(codec), [](AVCodecContext* c) { avcodec_close(c); av_free(c); });

    // we need to make a copy of videoStream->codec->extradata and give it to the context
    // make sure that this vector exists as long as the avVideoCodec exists
    std::vector<char> codecContextExtraData(stream->codec->extradata, stream->codec->extradata + stream->codec->extradata_size);
    avVideoCodec->extradata = reinterpret_cast<uint8_t*>(codecContextExtraData.data());
    avVideoCodec->extradata_size = codecContextExtraData.size();

    // initializing the structure by opening the codec
    if (avcodec_open2(avVideoCodec.get(), codec, nullptr) < 0)
        throw std::runtime_error("Could not open codec");

    // allocating an AVFrame
    std::shared_ptr<AVFrame> avFrame(avcodec_alloc_frame(), &av_free);

    // the current packet of data
    Packet packet;
    // data in the packet of data already processed
    size_t offsetInData = 0;

    // the decoding loop, running until EOF
    while (true) {
        // reading a packet using libavformat
        if (offsetInData >= packet.packet.size) {
            do {
                packet.reset(avFormat.get());
                if (packet.packet.stream_index != videoStream->index)
                    continue;
            } while(0);
        }

        // preparing the packet that we will send to libavcodec
        AVPacket packetToSend;
        packetToSend.data = packet.packet.data + offsetInData;
        packetToSend.size = packet.packet.size - offsetInData;

        // sending data to libavcodec
        int isFrameAvailable = 0;
        const auto processedLength = avcodec_decode_video2(avVideoCodec.get(), avFrame.get(), &isFrameAvailable, &packetToSend);
        if (processedLength < 0) {
            av_free_packet(&packet.packet);
            throw std::runtime_error("Error while processing the data");
        }
        offsetInData += processedLength;

        // processing the image if available
        if (isFrameAvailable) {
            // display image on the screen
            AVPicture pic;
            avpicture_alloc(&pic, PIX_FMT_RGB24, avFrame->width, avFrame->height);
            auto ctxt = sws_getContext(avFrame->width, avFrame->height, static_cast<PixelFormat>(avFrame->format), avFrame->width, avFrame->height, PIX_FMT_RGB24, SWS_BILINEAR, nullptr, nullptr, nullptr);
            if (ctxt == nullptr)
                throw std::runtime_error("Error while calling sws_getContext");
            sws_scale(ctxt, avFrame->data, avFrame->linesize, 0, avFrame->height, pic.data, pic.linesize);

            // pic.data[0] now contains the image data in RGB format (3 bytes)
            // and pic.linesize[0] is the pitch of the data (ie. size of a row in memory, which can be larger than width*sizeof(pixel))

            // we can for example upload it to an OpenGL texture (note: untested code)
            // glBindTexture(GL_TEXTURE_2D, myTex);
            // for (int i = 0; i < avFrame->height; ++i) {
            // 	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, i, avFrame->width, 1, GL_RGB, GL_UNSIGNED_BYTE, avFrame->data[0] + (i * pic.linesize[0]));
            // }

            avpicture_free(&pic);
            // sleeping until next frame
            const auto msToWait = avVideoCodec->ticks_per_frame * 1000 * avVideoCodec->time_base.num / avVideoCodec->time_base.den;
            std::this_thread::sleep_for(std::chrono::milliseconds(msToWait));
        }
    }
    return a.exec();
}
