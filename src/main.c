// Define the frame rate of the video
#define VIDEO_FPS 30

/** Default audio sample rate. */
#define AUDIO_SAMPLE_RATE 48000

/** Size of the audio buffer. */
#define AUDIO_BUFFER_SIZE 2048

/** Number of audio samples in the buffer. */
#define AUDIO_SAMPLES 1024

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

int main(int argc, char *argv[]) 
{
    // Declare pointers for input and output format contexts
    AVFormatContext *video_format_ctx = NULL;
    AVFormatContext *audio_format_ctx = NULL;
    AVFormatContext *out_format_ctx = NULL;

    AVStream *video_stream = NULL, *audio_stream = NULL;

    AVPacket pkt;
    int ret, stream_index;
    int *stream_mapping = NULL;
    int stream_mapping_size = 0;

    // Check if the correct number of arguments are passed (input and output file names)
    if (argc < 3) 
    {
        printf("usage: %s input output\n", argv[0]);
        return 1;
    }

    // Open the H26x video file and initialize the format context for it
    if(avformat_open_input(&video_format_ctx, argv[1], NULL, NULL))
    {
        fprintf(stderr, "Could not open the audio file\n");
        return -1;
    }

    // Open the AAC audio file and initialize the format context for it
    if (avformat_open_input(&audio_format_ctx, argv[2], NULL, NULL) < 0) 
    {
        fprintf(stderr, "Could not open the audio file\n");
        return -1;
    }

    // Allocate the output format context based on the output file name
    avformat_alloc_output_context2(&out_format_ctx, NULL, "mp4", "video.mp4");
    if (!out_format_ctx) 
    {
        printf("Could not create output context\n");
        return -1;
    }

    // Configurazione dello Stream Video (H.265/HEVC)
    const AVCodec *video_codec = avcodec_find_encoder_by_name("libx265");
    if (!video_codec) {
        fprintf(stderr, "H.265 codec not found\n");
        return -1;
    }

    video_stream = avformat_new_stream(out_format_ctx, video_codec);
    if (!video_stream) {
        fprintf(stderr, "Failed to create video stream\n");
        return -1;
    }

    video_stream->codecpar->codec_id = video_codec->id;
    video_stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    video_stream->codecpar->width = 1280;  // Set your video width
    video_stream->codecpar->height = 720; // Set your video height
    video_stream->codecpar->format = AV_PIX_FMT_YUV420P; // Common pixel format for H.265
    // video_stream->time_base = (AVRational){1, VIDEO_FPS}; // Set your video framerate

    // Configurazione dello Stream Audio (AAC)
    const AVCodec *audio_codec = avcodec_find_encoder_by_name("aac");
    if (!audio_codec) {
        fprintf(stderr, "AAC codec not found\n");
        return -1;
    }

    audio_stream = avformat_new_stream(out_format_ctx, audio_codec);
    if (!audio_stream) {
        fprintf(stderr, "Failed to create audio stream\n");
        return -1;
    }

    audio_stream->codecpar->codec_id = audio_codec->id;
    audio_stream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    audio_stream->codecpar->sample_rate = 48000; // Set your audio sample rate
    audio_stream->codecpar->format = AV_SAMPLE_FMT_FLTP; // Common format for AAC
    audio_stream->codecpar->frame_size = AUDIO_SAMPLES; 
    // audio_stream->time_base = (AVRational){1, audio_stream->codecpar->sample_rate};


    // Open the output file for writing
    avio_open(&out_format_ctx->pb, "video.mp4", AVIO_FLAG_WRITE);

    // Write the header of the output file
    ret = avformat_write_header(out_format_ctx, NULL);
    if (ret < 0) 
    {
        fprintf(stderr, "Error occurred when opening output file\n");
        return -1;
    }
    
    // Assume a framerate of 30 fps for simplicity, so the duration of a frame is 1/30th of a second
    int64_t video_frame_duration = 90000/VIDEO_FPS;
    int64_t audio_frame_duration = AUDIO_SAMPLES*90000/AUDIO_SAMPLE_RATE;
    int64_t video_next_pts = 0;
    int64_t audio_next_pts = 0;

    // Main loop for reading frames from the input and writing to the output
    while (1) 
    {

        // Read a frame from the input file
        {
            ret = av_read_frame(video_format_ctx, &pkt);
            if (ret < 0)
                break;

            // Update the stream index in the packet to the mapped index
            pkt.stream_index = 0;

            // Set timestamps for the packet
            pkt.pts = video_next_pts;
            pkt.dts = video_next_pts;
            // Increment the PTS for the next frame
            video_next_pts += video_frame_duration;

            // Write the frame to the output file
            av_interleaved_write_frame(out_format_ctx, &pkt);
            // Unreference the packet
            av_packet_unref(&pkt);
        }

        // Read a frame from the audio file
        {
            ret = av_read_frame(audio_format_ctx, &pkt);
            if (ret < 0)
                break;

            // Assume the audio stream is the first stream
            pkt.stream_index = 1;

            // Set PTS for the audio
            pkt.pts = audio_next_pts;
            pkt.dts = audio_next_pts;
            // Calculate the next PTS for audio based on the duration of an audio frame
            // (This will depend on the format and specific parameters of your audio file)
            audio_next_pts += audio_frame_duration;

            // Write the audio frame to the output file
            av_interleaved_write_frame(out_format_ctx, &pkt);

            av_packet_unref(&pkt);
        }

    }

    // Write the trailer to the output file
    av_write_trailer(out_format_ctx);

    // Close the input file and free the format context
    avformat_close_input(&video_format_ctx);

    // Remember to close the audio file and free its context at the end
    avformat_close_input(&audio_format_ctx);

}