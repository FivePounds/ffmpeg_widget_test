//#pragma execution_character_set("UTF-8")
#include "myffmpeg.h"
#include <QDebug>
#include <QApplication>
#include <QtConcurrent/QtConcurrent>

Myffmpeg::Myffmpeg(QObject *parent) : QObject(parent),m_stop(false),m_outBuffer(nullptr)
{
    m_vidoeStreamIndex = -1;
    //初始化ffmpeg
    av_register_all();//注册库中所有可用的文件格式和解码器
    avformat_network_init();//初始化网络流格式,使用RTSP网络流时必须先执行
    m_pAVFormatContext = nullptr;
    m_pAVFrameYUV = av_frame_alloc();
    m_pAVFrameRGB = av_frame_alloc();
    connect(&m_futureWatcher,&QFutureWatcher<void>::finished,this,&Myffmpeg::freeMemory);
}

Myffmpeg::~Myffmpeg()
{
    freeMemory();
    av_frame_free(&m_pAVFrameYUV);
    av_frame_free(&m_pAVFrameRGB);
    avformat_network_deinit();
}

bool Myffmpeg::init(const QString &url)
{
    //tcp连接
    AVDictionary *options = NULL;
    av_dict_set(&options, "buffer_size", "102400", 0);
    av_dict_set(&options, "max_delay", "500000", 0);
    av_dict_set(&options, "stimeout", "20000000", 0);  //设置超时断开连接时间
//    av_dict_set(&options, "rtsp_transport", "tcp", 0);  //以udp方式打开，如果以tcp方式打开将udp替换为tcp
    //打开视频流
    int result = avformat_open_input(&m_pAVFormatContext,url.toLocal8Bit().data(),NULL,&options);
    if(result < 0)
    {
        qDebug()<<"打开视频流失败";
        return false;
    }
    //获取视频流信息
    result = avformat_find_stream_info(m_pAVFormatContext,NULL);
    if(result < 0)
    {
        qDebug()<<"获取视频流信息失败";
        return false;
    }
    //获取视频流索引
    m_vidoeStreamIndex = -1;
    AVCodec *pAVCodec;
    m_vidoeStreamIndex = av_find_best_stream(m_pAVFormatContext,AVMEDIA_TYPE_VIDEO,-1,-1,&pAVCodec,0);
    if(m_vidoeStreamIndex == -1)
    {
        qDebug()<<"获取视频流索引失败";
        return false;
    }
    //获取视频流的分辨率大小
    m_pAVCodecContext = m_pAVFormatContext->streams[m_vidoeStreamIndex]->codec;
    m_videoWidth = m_pAVCodecContext->width;
    m_videoHeight = m_pAVCodecContext->height;
    m_outBuffer = (unsigned char*)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_RGB24,m_videoWidth,m_videoHeight,1));
    av_image_fill_arrays(m_pAVFrameRGB->data,m_pAVFrameRGB->linesize,m_outBuffer,AV_PIX_FMT_RGB24,m_videoWidth,m_videoHeight,1);

    m_pSwsContext = sws_getContext(m_videoWidth,m_videoHeight,m_pAVCodecContext->pix_fmt
                                   ,m_videoWidth,m_videoHeight,AV_PIX_FMT_RGB24,SWS_BICUBIC,0,0,0);
    //打开解码器
    result = avcodec_open2(m_pAVCodecContext,pAVCodec,0);
    if(result < 0)
    {
        qDebug()<<"打开解码器失败";
        return false;
    }
    qDebug()<<"init ffmpeg success";
    return true;
}

void Myffmpeg::play()
{
    //读取视频帧
    while (!m_stop && av_read_frame(m_pAVFormatContext,&m_AVPacket) >=0)
    {
        if(m_AVPacket.stream_index == m_vidoeStreamIndex)
        {   //解码
            if(avcodec_send_packet(m_pAVCodecContext,&m_AVPacket) >=0)
            {
                while (avcodec_receive_frame(m_pAVCodecContext,m_pAVFrameYUV) == 0)
                {
                    //格式转换
                    m_mutex.lock();
                    sws_scale(m_pSwsContext,(const uint8_t* const*)m_pAVFrameYUV->data,m_pAVFrameYUV->linesize
                              ,0,m_videoHeight,m_pAVFrameRGB->data,m_pAVFrameRGB->linesize);
                    QImage image(m_outBuffer,m_videoWidth,m_videoHeight,QImage::Format_RGB888);
                    m_latestImage= image;
//                    emit getFrame(image.copy());
                    m_mutex.unlock();
                }
            }
        }
        av_packet_unref(&m_AVPacket);
        QApplication::processEvents();
    }
    m_latestImage = m_latestImage.copy();
}

void Myffmpeg::startPlay(const QString &url)
{
    if(m_pAVFormatContext)
    {   //视频打开状态
        return;
    }
    m_stop = false;
    if(init(url))
    {
        //新线程中解码
        m_futureWatcher.setFuture(QtConcurrent::run(this,&Myffmpeg::play));
    }
    else {
        qDebug()<<"init failed";
    }
}

void Myffmpeg::stop()
{
    m_stop = true;
}

QImage Myffmpeg::getLatestImage()
{
    m_mutex.lock();
    QImage copyImage = m_latestImage.copy();
    m_mutex.unlock();
    return copyImage;
}

void Myffmpeg::freeMemory()
{
    if(m_outBuffer)
    {
        av_freep(&m_outBuffer);
    }
    sws_freeContext(m_pSwsContext);
    if(m_pAVFormatContext)
    {
        avformat_close_input(&m_pAVFormatContext);
    }
}
