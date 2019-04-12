#ifndef MYFFMPEG_H
#define MYFFMPEG_H

#include <QObject>
#include <QMutex>
#include <QImage>
#include <QFutureWatcher>
extern "C"
{
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libavfilter/avfilter.h"
    #include "libswscale/swscale.h"
    #include "libavutil/imgutils.h"
}



class Myffmpeg : public QObject
{
    Q_OBJECT
public:
    explicit Myffmpeg(QObject *parent = nullptr);
    ~Myffmpeg();
    void startPlay(const QString &url);
    void stop();
    QImage getLatestImage();
signals:
    void getFrame(QImage image);
protected:
    void freeMemory();
    bool init(const QString & url);
    void play();
private:
    int m_vidoeStreamIndex; //视频流位置
    AVFormatContext *m_pAVFormatContext;
    AVFrame *m_pAVFrameYUV;    //解码后的frame yuv格式
    AVFrame *m_pAVFrameRGB;   //格式转换后的frame rgb24格式
    unsigned char * m_outBuffer;
    AVPacket m_AVPacket;
    SwsContext *m_pSwsContext;
    AVCodecContext *m_pAVCodecContext;
    int m_videoWidth;
    int m_videoHeight;
    QMutex m_mutex;
    bool m_stop;
    QFutureWatcher<void> m_futureWatcher;
    QImage m_latestImage;
};

#endif // MYFFMPEG_H
