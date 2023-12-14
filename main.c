#define VIDEO_FPS 30

#include <libavformat/avformat.h>

int main(int argc, char *argv[]) {
    AVFormatContext *in_format_ctx = NULL, *out_format_ctx = NULL;
    AVPacket pkt;
    int ret, stream_index;
    int *stream_mapping = NULL;
    int stream_mapping_size = 0;

    if (argc < 3) {
        printf("usage: %s input output\n", argv[0]);
        return 1;
    }

    avformat_open_input(&in_format_ctx, argv[1], NULL, NULL);

    avformat_find_stream_info(in_format_ctx, NULL);

    avformat_alloc_output_context2(&out_format_ctx, NULL, NULL, argv[2]);
    if (!out_format_ctx) {
        printf("Could not create output context\n");
        return -1;
    }

    stream_mapping_size = in_format_ctx->nb_streams;
    stream_mapping = av_mallocz(stream_mapping_size * sizeof(*stream_mapping));

    for (int i = 0; i < in_format_ctx->nb_streams; i++) {
        AVStream *out_stream;
        AVStream *in_stream = in_format_ctx->streams[i];
        AVCodecParameters *in_codecpar = in_stream->codecpar;

        if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
            stream_mapping[i] = -1;
            continue;
        }

        stream_mapping[i] = stream_index++;

        out_stream = avformat_new_stream(out_format_ctx, NULL);
        avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
        out_stream->codecpar->codec_tag = 0;
    }

    avio_open(&out_format_ctx->pb, argv[2], AVIO_FLAG_WRITE);

    ret = avformat_write_header(out_format_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        return -1;
    }

    AVStream *in_stream, *out_stream;
    
    // Assumi un framerate di 30 fps per semplificare, quindi la durata di un frame è 1/30 di secondo
    int64_t frame_duration = 90000/VIDEO_FPS;
    int64_t next_pts = 0;

    while (1) 
    {
        ret = av_read_frame(in_format_ctx, &pkt);
        if (ret < 0)
            break;

        in_stream = in_format_ctx->streams[pkt.stream_index];
        if (pkt.stream_index >= stream_mapping_size || stream_mapping[pkt.stream_index] < 0) {
            av_packet_unref(&pkt);
            continue;
        }

        pkt.stream_index = stream_mapping[pkt.stream_index];
        out_stream = out_format_ctx->streams[pkt.stream_index];

        // Imposta i timestamp
        pkt.pts = next_pts;
        pkt.dts = next_pts;
        next_pts += frame_duration; // Incrementa il PTS per il prossimo frame

        av_interleaved_write_frame(out_format_ctx, &pkt);
        av_packet_unref(&pkt);
    }

    av_write_trailer(out_format_ctx);

    avformat_close_input(&in_format_ctx);

    avio_closep(&out_format_ctx->pb);
    avformat_free_context(out_format_ctx);

    av_freep(&stream_mapping);

    return 0;
}
