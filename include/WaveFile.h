#ifndef WAVEFILE_H
#define WAVEFILE_H

#include <string>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
// refer to http://blog.csdn.net/maverick1990/article/details/8996608

class WaveFile
{
public:
#pragma pack(push, 1)
    struct wavehead {
        char sign[4];                   // "RIFF" sign
        uint32_t flength;               // File length
        char wavesign[4];               // "WAVE" sign
        char fmtsign[4];                // "fmt" sign
        uint32_t unused;                // reserved
        uint16_t formattype;            // format type
        uint16_t channelnum;            // channel number, 1:mono 2:stereo
        uint32_t samplerate;            // sampling rate
        uint32_t transferrate;          // transfer rate
        uint16_t adjustnum;             // Block align
        uint16_t databitnum;            // Bits per sample
    } head;
#pragma pack(pop)
    /** Bytes of PCM represented by the decoded buffer (after load: `datanum * blockAlign`). */
    uint32_t datalength;
    /** Same as `datanum` after a successful read: number of sample frames decoded for channel 0. */
    uint32_t totalsample;
    /** Bits per sample per channel (from WAV `wBitsPerSample` or raw format). */
    uint32_t bitpersample;
    /** Number of `short` samples in `Data`: one per frame, first channel only — equals frame count. */
    uint32_t datanum;

    short *Data;

    WaveFile() : datalength(0), totalsample(0), bitpersample(0), datanum(0), Data(nullptr) {}

    ~WaveFile() {
        if (Data) {
            delete[] Data;
            Data = nullptr;
        }
    }

    WaveFile(const WaveFile& other)
        : head(other.head), datalength(other.datalength), totalsample(other.totalsample),
          bitpersample(other.bitpersample), datanum(other.datanum), Data(nullptr)
    {
        if (other.Data && other.datanum > 0) {
            Data = new short[datanum];
            std::copy(other.Data, other.Data + datanum, Data);
        }
    }

    WaveFile& operator=(const WaveFile& other) {
        if (this != &other) {
            delete[] Data;
            Data = nullptr;
            head = other.head;
            datalength = other.datalength;
            totalsample = other.totalsample;
            bitpersample = other.bitpersample;
            datanum = other.datanum;
            if (other.Data && other.datanum > 0) {
                Data = new short[datanum];
                std::copy(other.Data, other.Data + datanum, Data);
            }
        }
        return *this;
    }

    void WavInfo()
    {
        printf("--- Audio Info ---\n");
        printf("Type:        %.4s / %.4s\n", head.sign, head.wavesign);
        printf("Format type: %d%s\n", head.formattype,
               head.formattype == 6 ? " (A-law)" :
               head.formattype == 7 ? " (μ-law)" : " (PCM)");
        printf("Channels:    %d (%s)\n", head.channelnum,
               head.channelnum == 1 ? "mono" :
               head.channelnum == 2 ? "stereo" : "multi-channel");
        printf("Sample rate: %u Hz\n", head.samplerate);
        printf("Bit depth:   %u-bit\n", head.databitnum);
        printf("Data length: %u bytes\n", datalength);
        printf("Samples:     %u\n", datanum);
        printf("------------------\n");
    }

    bool WavRead(const std::string& filename)
    {
        if (Data) {
            delete[] Data;
            Data = nullptr;
        }
        datalength = 0;
        totalsample = 0;
        datanum = 0;
        bitpersample = 0;

        FILE *fp;

        if (NULL == (fp = fopen(filename.c_str(), "rb")))
        {
            printf("Cannot read wave file\n");
            return false;
        }

        if (fread(&head, sizeof(head), 1, fp) != 1) {
            fclose(fp);
            return false;
        }

        // Skip any extension bytes in the fmt chunk (size is in head.unused)
        if (head.unused > 16) {
            if (fseek(fp, (long)(head.unused - 16), SEEK_CUR) != 0) {
                fclose(fp);
                return false;
            }
        }

        // Scan through chunks until the "data" chunk is found
        char chunkId[4];
        uint32_t chunkSize = 0;
        bool foundData = false;
        while (fread(chunkId, 4, 1, fp) == 1) {
            if (fread(&chunkSize, 4, 1, fp) != 1) break;
            if (chunkId[0]=='d' && chunkId[1]=='a' &&
                chunkId[2]=='t' && chunkId[3]=='a') {
                datalength = chunkSize;
                foundData = true;
                break;
            }
            // Skip unknown chunk; WAV pads odd-size chunks to even boundary
            long skip = (long)chunkSize + (chunkSize & 1);
            if (fseek(fp, skip, SEEK_CUR) != 0) break;
        }
        if (!foundData) {
            printf("Cannot find 'data' chunk in WAV file\n");
            fclose(fp);
            return false;
        }

        if (head.adjustnum == 0) {
            printf("Invalid WAV: block align is zero\n");
            fclose(fp);
            return false;
        }

        totalsample = datalength / head.adjustnum;
        // bitsPerSample in WAV is per channel; we decode one sample (first channel) per frame
        bitpersample = head.databitnum;
        const uint32_t expectedFrames = totalsample;

        if (head.databitnum != 8 && head.databitnum != 16) {
            printf("Unsupported bit depth: %d-bit. Only 8-bit and 16-bit PCM are supported.\n",
                   head.databitnum);
            datalength = 0;
            totalsample = 0;
            bitpersample = 0;
            fclose(fp);
            return false;
        }
        // formattype: 1=PCM, 6=A-law, 7=μ-law
        if (head.formattype != 1 && head.formattype != 6 && head.formattype != 7) {
            printf("Unsupported WAV format type: %d\n", head.formattype);
            datalength = 0;
            totalsample = 0;
            bitpersample = 0;
            fclose(fp);
            return false;
        }

        const unsigned bytesPerChannel = head.databitnum / 8;
        if (bytesPerChannel == 0 || head.databitnum % 8 != 0) {
            printf("Invalid WAV: bits per sample must be a multiple of 8\n");
            datalength = 0;
            totalsample = 0;
            bitpersample = 0;
            fclose(fp);
            return false;
        }
        const long skipRestOfFrame =
            (head.channelnum > 1)
                ? (long)head.adjustnum - (long)bytesPerChannel
                : 0;
        if (skipRestOfFrame < 0) {
            printf("Invalid WAV: block align smaller than one channel sample\n");
            datalength = 0;
            totalsample = 0;
            bitpersample = 0;
            fclose(fp);
            return false;
        }
        if (head.channelnum > 1 && skipRestOfFrame == 0) {
            printf("Invalid WAV: multi-channel yet block align matches single channel\n");
            datalength = 0;
            totalsample = 0;
            bitpersample = 0;
            fclose(fp);
            return false;
        }

        Data = new short[expectedFrames];
        uint32_t n = 0;

        if (16 == head.databitnum) {
            // WAV PCM sample data is little-endian per spec (regardless of host endianness)
            for (; n < expectedFrames; ) {
                uint8_t byte0 = 0, byte1 = 0; // LE: byte0 = LSB, byte1 = MSB
                if (fread(&byte0, 1, 1, fp) != 1)
                    break;
                if (fread(&byte1, 1, 1, fp) != 1)
                    break;
                const short sample = pcm16le(byte0, byte1);
                if (head.channelnum > 1) {
                    if (skipRestOfFrame > 0 && fseek(fp, skipRestOfFrame, SEEK_CUR) != 0)
                        break;
                }
                Data[n++] = sample;
            }
        }
        else { // 8-bit: PCM unsigned, A-law (6), or μ-law (7)
            for (; n < expectedFrames; ) {
                uint8_t val = 0;
                if (fread(&val, 1, 1, fp) != 1)
                    break;
                short sample;
                if (head.formattype == 6)
                    sample = alaw2linear(val);
                else if (head.formattype == 7)
                    sample = ulaw2linear(val);
                else
                    sample = static_cast<short>((static_cast<int>(val) - 128) * 256);
                if (head.channelnum > 1) {
                    if (skipRestOfFrame > 0 && fseek(fp, skipRestOfFrame, SEEK_CUR) != 0)
                        break;
                }
                Data[n++] = sample;
            }
        }

        fclose(fp);

        if (n == 0) {
            delete[] Data;
            Data = nullptr;
            datanum = 0;
            totalsample = 0;
            printf("No samples decoded from WAV\n");
            return false;
        }

        if (n < expectedFrames) {
            short *trimmed = new short[n];
            std::copy(Data, Data + n, trimmed);
            delete[] Data;
            Data = trimmed;
            printf("Warning: incomplete WAV read (%u of %u frames)\n", n, expectedFrames);
        }

        datanum = totalsample = n;
        uint32_t decodedBytes = 0;
        if (!decodedBytesFit(n, head.adjustnum, &decodedBytes)) {
            printf("Invalid WAV: decoded byte length overflow\n");
            delete[] Data;
            Data = nullptr;
            datanum = totalsample = datalength = 0;
            return false;
        }
        datalength = decodedBytes;
        return true;
    }

    struct RawPcmFormat {
        enum class Encoding { Linear, ALaw, ULaw };
        uint32_t sampleRate;
        uint16_t bitsPerSample; // 8 or 16
        uint16_t channels;      // 1 or 2
        bool     littleEndian;
        Encoding encoding = Encoding::Linear;
    };

    // ITU-T G.711 μ-law decode
    static short ulaw2linear(uint8_t u) {
        u = ~u;
        int sign     = (u & 0x80) ? -1 : 1;
        int exponent = (u >> 4) & 0x07;
        int mantissa = u & 0x0F;
        int sample   = ((mantissa | 0x10) << (exponent + 3)) - 132;
        return static_cast<short>(sign * sample);
    }

    // ITU-T G.711 A-law decode
    static short alaw2linear(uint8_t a) {
        a ^= 0x55;
        int sign     = (a & 0x80) ? 1 : -1;
        int mag      = a & 0x7F;
        int exponent = mag >> 4;
        int mantissa = mag & 0x0F;
        int sample   = (exponent == 0)
            ? (mantissa << 1) | 1
            : ((0x10 | mantissa) << exponent) | (1 << (exponent - 1));
        return static_cast<short>(sign * (sample << 3));
    }

    bool RawRead(const std::string& filename, const RawPcmFormat& fmt)
    {
        if (Data) {
            delete[] Data;
            Data = nullptr;
        }
        datalength = 0;
        totalsample = 0;
        datanum = 0;
        bitpersample = 0;

        if (fmt.bitsPerSample != 8 && fmt.bitsPerSample != 16) {
            printf("Unsupported bit depth: %d-bit.\n", fmt.bitsPerSample);
            return false;
        }

        FILE *fp = fopen(filename.c_str(), "rb");
        if (!fp) {
            printf("Cannot open file: %s\n", filename.c_str());
            return false;
        }

        // Determine file size
        fseek(fp, 0, SEEK_END);
        long fileSize = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        if (fileSize <= 0) {
            fclose(fp);
            return false;
        }

        // Populate a synthetic header so the rest of the code can work uniformly
        // Zero out the entire struct first to clear any residue from a failed WavRead()
        head = {};
        std::memcpy(head.sign,     "RAW ", 4);
        std::memcpy(head.wavesign, "PCM ", 4);
        std::memcpy(head.fmtsign,  "fmt ", 4);
        if (fmt.channels < 1) {
            fclose(fp);
            return false;
        }
        uint16_t blockAlign = static_cast<uint16_t>((fmt.bitsPerSample / 8) * fmt.channels);
        if (blockAlign == 0) {
            fclose(fp);
            return false;
        }
        head.formattype  = 1; // PCM
        head.channelnum  = fmt.channels;
        head.samplerate  = fmt.sampleRate;
        head.databitnum  = fmt.bitsPerSample;
        head.adjustnum   = blockAlign;
        head.transferrate = fmt.sampleRate * blockAlign;

        datalength   = static_cast<uint32_t>(fileSize);
        bitpersample = fmt.bitsPerSample;
        totalsample  = datalength / blockAlign;
        const uint32_t expectedFrames = totalsample;

        const unsigned bytesPerChannel = fmt.bitsPerSample / 8;
        const long skipRestOfFrame =
            (fmt.channels > 1)
                ? (long)blockAlign - (long)bytesPerChannel
                : 0;
        if (skipRestOfFrame < 0) {
            datalength = 0;
            totalsample = 0;
            bitpersample = 0;
            fclose(fp);
            return false;
        }
        if (fmt.channels > 1 && skipRestOfFrame == 0) {
            printf("Invalid raw PCM: multi-channel but block align matches single channel\n");
            datalength = 0;
            totalsample = 0;
            bitpersample = 0;
            fclose(fp);
            return false;
        }

        Data = new short[expectedFrames];
        uint32_t n = 0;

        if (fmt.bitsPerSample == 8) {
            for (; n < expectedFrames; ) {
                uint8_t val = 0;
                if (fread(&val, 1, 1, fp) != 1)
                    break;
                short sample;
                switch (fmt.encoding) {
                    case RawPcmFormat::Encoding::ALaw:
                        sample = alaw2linear(val); break;
                    case RawPcmFormat::Encoding::ULaw:
                        sample = ulaw2linear(val); break;
                    default:
                        sample = static_cast<short>((static_cast<int>(val) - 128) * 256);
                }
                if (fmt.channels > 1) {
                    if (skipRestOfFrame > 0 && fseek(fp, skipRestOfFrame, SEEK_CUR) != 0)
                        break;
                }
                Data[n++] = sample;
            }
        } else { // 16-bit
            for (; n < expectedFrames; ) {
                uint8_t byte0 = 0, byte1 = 0;
                if (fread(&byte0, 1, 1, fp) != 1)
                    break;
                if (fread(&byte1, 1, 1, fp) != 1)
                    break;
                short sample = fmt.littleEndian
                    ? static_cast<short>((static_cast<uint16_t>(byte1) << 8) | byte0)
                    : static_cast<short>((static_cast<uint16_t>(byte0) << 8) | byte1);
                if (fmt.channels > 1) {
                    if (skipRestOfFrame > 0 && fseek(fp, skipRestOfFrame, SEEK_CUR) != 0)
                        break;
                }
                Data[n++] = sample;
            }
        }

        fclose(fp);

        if (n == 0) {
            delete[] Data;
            Data = nullptr;
            datanum = 0;
            totalsample = 0;
            printf("No samples decoded from raw PCM\n");
            return false;
        }

        if (n < expectedFrames) {
            short *trimmed = new short[n];
            std::copy(Data, Data + n, trimmed);
            delete[] Data;
            Data = trimmed;
            printf("Warning: incomplete raw PCM read (%u of %u frames)\n", n, expectedFrames);
        }

        datanum = totalsample = n;
        uint32_t decodedBytes = 0;
        if (!decodedBytesFit(n, blockAlign, &decodedBytes)) {
            printf("Invalid raw PCM: decoded byte length overflow\n");
            delete[] Data;
            Data = nullptr;
            datanum = totalsample = datalength = 0;
            return false;
        }
        datalength = decodedBytes;
        return true;
    }

private:
    /** LE 16-bit PCM: byte0 is low byte, byte1 is high byte. */
    static short pcm16le(uint8_t byte0, uint8_t byte1)
    {
        return static_cast<short>((static_cast<uint16_t>(byte1) << 8) | byte0);
    }

    /** False if `frames * blockAlign` exceeds uint32_t (would truncate `datalength`). */
    static bool decodedBytesFit(uint32_t frames, uint32_t blockAlign, uint32_t *outDecodedBytes)
    {
        const uint64_t product = static_cast<uint64_t>(frames) * static_cast<uint64_t>(blockAlign);
        if (product > UINT32_MAX)
            return false;
        if (outDecodedBytes)
            *outDecodedBytes = static_cast<uint32_t>(product);
        return true;
    }
};

#endif // WAVEFILE_H
