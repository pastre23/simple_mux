// Define the frame rate of the video
#define VIDEO_FPS 30

#include <libavformat/avformat.h>

int main(int argc, char *argv[]) 
{
    // Declare pointers for input and output format contexts
    AVFormatContext *in_format_ctx = NULL, *out_format_ctx = NULL;
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

    // Open the input file and initialize the format context for it
    avformat_open_input(&in_format_ctx, argv[1], NULL, NULL);

    // Retrieve stream information for the input file
    avformat_find_stream_info(in_format_ctx, NULL);

    // Allocate the output format context based on the output file name
    avformat_alloc_output_context2(&out_format_ctx, NULL, NULL, argv[2]);
    if (!out_format_ctx) 
    {
        printf("Could not create output context\n");
        return -1;
    }

    // Allocate memory for stream mapping array based on the number of streams in the input file
    stream_mapping_size = in_format_ctx->nb_streams;
    stream_mapping = av_mallocz(stream_mapping_size * sizeof(*stream_mapping));

    // Loop through each stream in the input file
    for (int i = 0; i < in_format_ctx->nb_streams; i++) 
    {
        AVStream *out_stream;
        AVStream *in_stream = in_format_ctx->streams[i];
        AVCodecParameters *in_codecpar = in_stream->codecpar;

        // Filter out streams that are not audio, video, or subtitles
        if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) 
        {
            stream_mapping[i] = -1;
            continue;
        }

        // Map the stream index from input to output
        stream_mapping[i] = stream_index++;

        // Create a new stream in the output file and copy codec parameters from input
        out_stream = avformat_new_stream(out_format_ctx, NULL);
        avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
        out_stream->codecpar->codec_tag = 0;
    }

    // Open the output file for writing
    avio_open(&out_format_ctx->pb, argv[2], AVIO_FLAG_WRITE);

    // Write the header of the output file
    ret = avformat_write_header(out_format_ctx, NULL);
    if (ret < 0) 
    {
        fprintf(stderr, "Error occurred when opening output file\n");
        return -1;
    }

    AVStream *in_stream, *out_stream;
    
    // Assume a framerate of 30 fps for simplicity, so the duration of a frame is 1/30th of a second
    int64_t frame_duration = 90000/VIDEO_FPS;
    int64_t next_pts = 0;

    // Main loop for reading frames from the input and writing to the output
    while (1) 
    {
        // Read a frame from the input file
        ret = av_read_frame(in_format_ctx, &pkt);
        if (ret < 0)
            break;

        // Get the corresponding input stream
        in_stream = in_format_ctx->streams[pkt.stream_index];
        // Check if the stream is mapped, if not, unreference the packet and continue
        if (pkt.stream_index >= stream_mapping_size || stream_mapping[pkt.stream_index] < 0) 
        {
            av_packet_unref(&pkt);
            continue;
        }

        // Update the stream index in the packet to the mapped index
        pkt.stream_index = stream_mapping[pkt.stream_index];
        // Get the corresponding output stream
        out_stream = out_format_ctx->streams[pkt.stream_index];

        // Set timestamps for the packet
        pkt.pts = next_pts;
        pkt.dts = next_pts;
        // Increment the PTS for the next frame
        next_pts += frame_duration;

        // Write the frame to the output file
        av_interleaved_write_frame(out_format_ctx, &pkt);
        // Unreference the packet
        av_packet_unref(&pkt);
    }

    // Write the trailer to the output file
    av_write_trailer(out_format_ctx);

    // Close the input file and free the format context
    avformat_close_input(&in_format_ctx);

}