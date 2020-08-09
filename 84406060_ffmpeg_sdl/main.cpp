#include <stdio.h>
#include <string.h>
extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/imgutils.h>
    #include <SDL/SDL.h>
    #include <SDL/SDL_video.h>
    #include <libswscale/swscale.h>
}

int main(int argc, char** argv) {
    AVFormatContext *pFormatCtx;
    int i, videoindex;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    char filepath[] = "test.mp4";

    avformat_network_init();
    pFormatCtx = avformat_alloc_context();
    if(avformat_open_input(&pFormatCtx, filepath, NULL, NULL) < 0) {
        printf("open error");
        return -1;
    }
    if(avformat_find_stream_info(pFormatCtx, NULL)) {
        printf("find stream error.");
        return -1;
    }
    videoindex = -1;
    for(int i = 0; i < pFormatCtx->nb_streams; i++){
        if(pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            videoindex = i;
            break;
        }
    }
    if(videoindex == -1) {
        printf("find video codec error.\n");
        return -1;
    }
    pCodecCtx = pFormatCtx->streams[videoindex]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec == NULL) {
        printf("find codec error.");
        return -1;
    }
    if(avcodec_open2(pCodecCtx, pCodec, NULL)){
        printf("open codec error.\n");
        return -1;
    }
    AVFrame *pFrame, *pFrameYUV;
    pFrame = av_frame_alloc();
    pFrameYUV = av_frame_alloc();
    uint8_t *out_buffer;
    int buffer_size = avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
    out_buffer = (uint8_t *)malloc(buffer_size);
    av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, 
        out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        printf("sdl init error.");
        return -1;
    }
    SDL_Surface *screen;
    screen = SDL_SetVideoMode(pCodecCtx->width, pCodecCtx->height, 0, 0);
    if(!screen) {  
		printf("SDL: could not set video mode - exiting\n");  
		return -1;
	}

    SDL_Overlay *bmp;
    bmp = SDL_CreateYUVOverlay(pCodecCtx->width, pCodecCtx->height, 
                SDL_YV12_OVERLAY, screen);
    if(bmp == nullptr) {
        printf("bmp create error\n");
        return -1;
    }
    SDL_Rect rect;
    int ret, got_picture;
    static struct SwsContext *img_concert_ctx;
    int y_size = pCodecCtx->width * pCodecCtx->height;
    AVPacket avpacket;
    av_dump_format(pFormatCtx, 8, filepath, 0);
    while(0 <= ret) {
        ret = av_read_frame(pFormatCtx, &avpacket);
        if(avpacket.stream_index == videoindex) {
            ret = avcodec_send_packet(pCodecCtx, &avpacket);
            if(0 > ret) {
                if(ret == AVERROR_EOF){
                    printf("end...\n");
                    return 0;
                } else if (ret == AVERROR(EAGAIN)) {
                    continue;
                } else {
                    printf("decode error...\n");
                    return -1;
                }
            }
            while(avcodec_receive_frame(pCodecCtx, pFrame) >= 0){
                // img_concert_ctx = sws_getContext(   pCodecCtx->width, 
                //                                 pCodecCtx->height, 
                //                                 pCodecCtx->pix_fmt, 
                //                                 pCodecCtx->width, 
                //                                 pCodecCtx->height, 
                //                                 AV_PIX_FMT_YUV420P, 
                //                                 SWS_BICUBIC, NULL, NULL, NULL);
                // sws_scale(img_concert_ctx, (const uint8_t *const *)pFrame->data, pFrame->linesize, 
                //                     0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
                // sws_freeContext(img_concert_ctx);
                SDL_LockYUVOverlay(bmp);
				// bmp->pixels[0]=pFrameYUV->data[0];
				// bmp->pixels[2]=pFrameYUV->data[1];
				// bmp->pixels[1]=pFrameYUV->data[2];     
				// bmp->pitches[0]=pFrameYUV->linesize[0];
				// bmp->pitches[2]=pFrameYUV->linesize[1];   
				// bmp->pitches[1]=pFrameYUV->linesize[2];
				pFrameYUV->data[0] = bmp->pixels[0];
                pFrameYUV->data[1] = bmp->pixels[2];
                pFrameYUV->data[2] = bmp->pixels[1];
            
                pFrameYUV->linesize[0] = bmp->pitches[0];
                pFrameYUV->linesize[1] = bmp->pitches[2];
                pFrameYUV->linesize[2] = bmp->pitches[1];
				SDL_UnlockYUVOverlay(bmp); 
                img_concert_ctx = sws_getContext(   pCodecCtx->width, 
                                pCodecCtx->height, 
                                pCodecCtx->pix_fmt, 
                                pCodecCtx->width, 
                                pCodecCtx->height, 
                                AV_PIX_FMT_YUV420P, 
                                SWS_BICUBIC, NULL, NULL, NULL);
                sws_scale(img_concert_ctx, (const uint8_t *const *)pFrame->data, pFrame->linesize, 
                                    0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
                sws_freeContext(img_concert_ctx);
                rect.x = 0;    
				rect.y = 0;    
				rect.w = pCodecCtx->width;    
				rect.h = pCodecCtx->height;    
				SDL_DisplayYUVOverlay(bmp, &rect);
				//延时40ms
				SDL_Delay(40);
                av_frame_unref(pFrame);
            }
        }
    }
    delete[] out_buffer;
	av_free(pFrameYUV);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

    return 0;
}
