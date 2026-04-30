#ifndef WAVEFILE_H
#define WAVEFILE_H

#include <string>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
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
    uint32_t datalength;
    uint32_t totalsample;
    uint32_t bitpersample;
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
            Data = new short[datanum + 10];
            std::copy(other.Data, other.Data + datanum + 10, Data);
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
                Data = new short[datanum + 10];
                std::copy(other.Data, other.Data + datanum + 10, Data);
            }
        }
        return *this;
    }

    void WavInfo()
    {
        printf("sign: %c%c%c%c\n",
               head.sign[0], head.sign[1], head.sign[2], head.sign[3]);
        printf("File length: %u\n", head.flength);
        printf("wave sign: %c%c%c%c\n",
               head.wavesign[0], head.wavesign[1],
               head.wavesign[2], head.wavesign[3]);
        printf("FMT sign:%c%c%c%c\n",
               head.fmtsign[0], head.fmtsign[1],
               head.fmtsign[2], head.fmtsign[3]);
        printf("Format type: %d\n", head.formattype);
        printf("Channel num: %d\n", head.channelnum);
        printf("Sample rate: %u\n", head.samplerate);
    }

    bool WavRead(const std::string& filename)
    {
        if (Data) {
            delete[] Data;
            Data = nullptr;
        }

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

        totalsample = datalength / head.adjustnum;
        bitpersample = head.databitnum / head.channelnum;
        datanum = totalsample * bitpersample / 16;

        if (head.databitnum != 8 && head.databitnum != 16) {
            printf("Unsupported bit depth: %d-bit. Only 8-bit and 16-bit PCM are supported.\n",
                   head.databitnum);
            fclose(fp);
            return false;
        }

        Data = new short[datanum + 10];
        if (16 == head.databitnum) {
            for (unsigned long i=0; !feof(fp) && i<datanum; i++) {
                fread(&Data[i], 2, 1, fp);
                // Skip 2nd channel for stereo
                if (2 == head.channelnum) {
                    fseek(fp, 2, SEEK_CUR);
                }
            }
        }
        else {
            for (unsigned long i=0; !feof(fp) && i<datanum; i++) {
                short low, high;

                fread(&low, 1, 1, fp);
                if (2 == head.channelnum) {
                    fseek(fp, 1, SEEK_CUR);
                }
                fread(&high, 1, 1, fp);
                if (2 == head.channelnum) {
                    fseek(fp, 1, SEEK_CUR);
                }
                Data[i] = (low & 0x00ff) | (high << 8 & 0xff00);
            }
        }

        fclose(fp);
        return true;
    }
};

#endif // WAVEFILE_H
