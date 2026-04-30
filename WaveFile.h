#ifndef WAVEFILE_H
#define WAVEFILE_H

#include <string>
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

    WaveFile() : Data(nullptr) {}
    ~WaveFile() {
        if (Data) {
            delete[] Data;
            Data = nullptr;
        }
    }

    void WavInfo()
    {
        printf("sign: %c%c%c%c\n",
               head.sign[0], head.sign[1], head.sign[2], head.sign[3]);
        printf("File length: %u\n", head.flength);
        printf("wave sign: %c%c%c%c",
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

        fread(&head, sizeof(head), 1, fp);
        char datasign[4];
        fread(datasign, 4, 1, fp);
        fread(&datalength, 4, 1, fp);

        totalsample = datalength / head.adjustnum;
        bitpersample = head.databitnum / head.channelnum;
        datanum = totalsample * bitpersample / 16;

        Data = new short[datanum + 10];
        if (0 == bitpersample) {
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
