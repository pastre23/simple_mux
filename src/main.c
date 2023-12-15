// Define the frame rate of the video
#define VIDEO_FPS 30

/** Default audio sample rate. */
#define AUDIO_SAMPLE_RATE 48000

/** Size of the audio buffer. */
#define AUDIO_BUFFER_SIZE 2048

/** Number of audio samples in the buffer. */
#define AUDIO_SAMPLES AUDIO_BUFFER_SIZE/2

#include <libavformat/avformat.h>

int main(int argc, char *argv[]) 
{
    // Declare pointers for input and output format contexts
    AVFormatContext *video_format_ctx = NULL;
    AVFormatContext *audio_format_ctx = NULL;
    AVFormatContext *out_format_ctx = NULL;

    AVPacket pkt;
    int ret, stream_index;
    int *stream_mapping = NULL;
    int stream_mapping_size = 0;

    // Check if the correct number of arguments are passed (input and output file names)
    if (argc < 4) 
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

    // Retrieve stream information for the input file
    if(avformat_find_stream_info(video_format_ctx, NULL))
    {
        fprintf(stderr, "Could not find stream information for the video file\n");
        return -1;
    }

    // Find stream information for the audio file
    if(avformat_find_stream_info(audio_format_ctx, NULL) < 0) 
    {
        fprintf(stderr, "Could not find stream information for the audio file\n");
        return -1;
    }

    // Allocate the output format context based on the output file name
    avformat_alloc_output_context2(&out_format_ctx, NULL, NULL, argv[3]);
    if (!out_format_ctx) 
    {
        printf("Could not create output context\n");
        return -1;
    }

    // Allocate memory for stream mapping array based on the number of streams in the input file
    stream_mapping_size = video_format_ctx->nb_streams;
    stream_mapping = av_mallocz(stream_mapping_size * sizeof(*stream_mapping));

    // Loop through each stream in the input file
    for (int i = 0; i < video_format_ctx->nb_streams; i++) 
    {
        AVStream *out_video_stream;
        AVStream *in_video_stream = video_format_ctx->streams[i];
        AVCodecParameters *video_codecpar = in_video_stream->codecpar;

        // Filter out streams that are not audio, video, or subtitles
        if (video_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
            video_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
            video_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) 
        {
            stream_mapping[i] = -1;
            continue;
        }

        // Map the stream index from input to output
        stream_mapping[i] = stream_index++;

        // Create a new stream in the output file and copy codec parameters from input
        out_video_stream = avformat_new_stream(out_format_ctx, NULL);
        avcodec_parameters_copy(out_video_stream->codecpar, video_codecpar);
        out_video_stream->codecpar->codec_tag = 0;
    }

    // Audio stream analysis
    // {
    //     // Assuming the first stream in the AAC file is audio
    //     AVStream *audio_stream = audio_format_ctx->streams[0];

    //     // Add an audio stream to the output
    //     AVStream *out_audio_stream = avformat_new_stream(out_format_ctx, NULL);
    //     if (!out_audio_stream) 
    //     {
    //         fprintf(stderr, "Failed to create an output audio stream\n");
    //         return -1;
    //     }
    //     // Copy codec parameters from the audio stream to the output audio stream
    //     avcodec_parameters_copy(out_audio_stream->codecpar, audio_stream->codecpar);
    //     // Ensure codec tag is set to 0 for the output stream
    //     out_audio_stream->codecpar->codec_tag = 0;
    // }

    // Open the output file for writing
    avio_open(&out_format_ctx->pb, argv[3], AVIO_FLAG_WRITE);

    // Write the header of the output file
    ret = avformat_write_header(out_format_ctx, NULL);
    if (ret < 0) 
    {
        fprintf(stderr, "Error occurred when opening output file\n");
        return -1;
    }

    AVStream *video_stream, *out_stream;
    
    // Assume a framerate of 30 fps for simplicity, so the duration of a frame is 1/30th of a second
    int64_t video_frame_duration = 90000/VIDEO_FPS;
    int64_t audio_frame_duration = AUDIO_SAMPLES*90000/AUDIO_SAMPLE_RATE;
    int64_t video_next_pts = 0;
    int64_t audio_next_pts = 0;

    // Main loop for reading frames from the input and writing to the output
    while (1) 
    {
        // Read a frame from the input file
        ret = av_read_frame(video_format_ctx, &pkt);
        if (ret < 0)
            break;

        // Get the corresponding input stream
        video_stream = video_format_ctx->streams[pkt.stream_index];
        // Check if the stream is mapped, if not, unreference the packet and continue
        if (pkt.stream_index >= stream_mapping_size || stream_mapping[pkt.stream_index] < 0) 
        {
            av_packet_unref(&pkt);
            continue;
        }

        {
            // Update the stream index in the packet to the mapped index
            pkt.stream_index = stream_mapping[pkt.stream_index];
            // Get the corresponding output stream
            out_stream = out_format_ctx->streams[pkt.stream_index];

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

        // // Read a frame from the audio file
        // {
        //     ret = av_read_frame(audio_format_ctx, &pkt);
        //     if (ret < 0)
        //         break;

        //     // Assume the audio stream is the first stream
        //     pkt.stream_index = 0;

        //     // Set PTS for the audio
        //     pkt.pts = audio_next_pts;
        //     pkt.dts = audio_next_pts;
        //     // Calculate the next PTS for audio based on the duration of an audio frame
        //     // (This will depend on the format and specific parameters of your audio file)
        //     audio_next_pts += audio_frame_duration;

        //     // Write the audio frame to the output file
        //     av_interleaved_write_frame(out_format_ctx, &pkt);

        //     av_packet_unref(&pkt);
        // }





    }

    // Write the trailer to the output file
    av_write_trailer(out_format_ctx);

    // Close the input file and free the format context
    avformat_close_input(&video_format_ctx);

    // // Remember to close the audio file and free its context at the end
    // avformat_close_input(&audio_format_ctx);

}