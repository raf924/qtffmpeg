#include "videothread.h"
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

#include <QDebug>

VideoThread::VideoThread(Widget *w, QObject *parent) :
    QThread(parent),
    _w(w)
{
}

void VideoThread::run()
{
    qDebug()<<"Starting thread";
    AVFormatContext *pFormatCtx = NULL;
    int             i, videoStream;
    AVCodecContext  *pCodecCtx;
    AVCodec         *pCodec;
    AVFrame         *pFrame;
    AVFrame         *pFrameRGB;
    AVPacket        packet;
    int             frameFinished;
    int             numBytes;
    uint8_t         *buffer;

    qDebug()<<"Structures initialized";

    if(!_w->filename) {
        qDebug()<<"Please provide a movie file\n";
        exit(1);
    }
    // Register all formats and codecs
    av_register_all();

    qDebug()<<"Codecs registered";

    // Open video file
    qDebug()<<_w->filename;
    int err = avformat_open_input(&pFormatCtx, _w->filename, NULL,NULL);
    if(err<0){
        qDebug()<<"Couldn't open file";
        char * errstr = new char[255];
        av_strerror(err,errstr,255);
        qDebug()<<errstr;
        exit(1); // Couldn't open file
    }
    qDebug()<<"Video file opened";
    // Retrieve stream information
    if(avformat_find_stream_info(pFormatCtx,NULL)<0){
        qDebug()<<"Couldn't find stream information";
        exit(1); // Couldn't find stream information
    }

    // Dump information about file onto standard error
    dump_format(pFormatCtx, 0, _w->filename, 0);

    // Find the first video stream
    videoStream=-1;
    for(i=0; i<pFormatCtx->nb_streams; i++)
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
            videoStream=i;
            break;
        }
    if(videoStream==-1){
        qDebug()<<"Didn't find a video stream";
        exit(1); // Didn't find a video stream
    }

    // Get a pointer to the codec context for the video stream
    pCodecCtx=pFormatCtx->streams[videoStream]->codec;

    // Find the decoder for the video stream
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL) {
        qDebug()<<"Unsupported codec!\n";
        exit(-1); // Codec not found
    }
    // Open codec
    if(avcodec_open2(pCodecCtx, pCodec, NULL)<0){
        qDebug()<<"Could not open codec";
        exit(-1); // Could not open codec
    }

    // Allocate video frame
    pFrame=avcodec_alloc_frame();

    // Allocate an AVFrame structure
    pFrameRGB=avcodec_alloc_frame();
    if(pFrameRGB==NULL)
    {
        qDebug()<<"Could not allocate frame";
        exit(-1);
    }

    // Determine required buffer size and allocate buffer
    numBytes = avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width,
                                pCodecCtx->height);
    buffer= new uint8_t[numBytes+64];
    int headerlen = sprintf((char *) buffer,"P6\n%d %d\n255\n",pCodecCtx->width,pCodecCtx->height);
    numBytes += headerlen;

    _w->setImage((uchar*)buffer,numBytes);

    // Assign appropriate parts of buffer to image planes in pFrameRGB


    // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
    // of AVPicture
    avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24,
                   pCodecCtx->width, pCodecCtx->height);
    SwsContext *img_convert_ctx;
    img_convert_ctx = sws_getContext(
                pCodecCtx->width, pCodecCtx->height,
                pCodecCtx->pix_fmt,
                pCodecCtx->width,pCodecCtx->height, PIX_FMT_RGB24,
                SWS_BICUBIC, NULL, NULL, NULL);

    // Read frames and save first five frames to disk
    i=0;
    while(av_read_frame(pFormatCtx, &packet)>=0) {
        // Is this a packet from the video stream?
        if(packet.stream_index==videoStream) {
            // Decode video frame
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished,
                                  &packet);

            // Did we get a video frame?
            if(frameFinished) {
                // Convert the image from its native format to RGB
                /*img_convert((AVPicture *)pFrameRGB, PIX_FMT_RGB32,
                    (AVPicture*)pFrame, pCodecCtx->pix_fmt, pCodecCtx->width,
                    pCodecCtx->height);*/
                qDebug()<<"Frame finished";
                sws_scale(img_convert_ctx,
                          pFrame->data,
                          pFrame->linesize,
                          0,
                          pCodecCtx->height,
                          pFrameRGB->data,
                          pFrameRGB->linesize);
                emit frameReady();
                sleep(1);
            }
        }

        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }

    // Free the RGB image
    av_free(buffer);
    av_free(pFrameRGB);

    // Free the YUV frame
    av_free(pFrame);

    // Close the codec
    avcodec_close(pCodecCtx);

    // Close the video file
    avformat_close_input(&pFormatCtx);
}
