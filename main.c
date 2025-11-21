#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "image.h"   // IMG_W, IMG_H, IMG_N, gImageGray buradan geliyor

int main(void)
{
    /* Çalışma bufferları */
    uint8_t img[IMG_N];
    uint8_t tmp[IMG_N];

    /* Histogram dizileri */
    uint32_t hist[256];
    uint32_t histEq[256];

    /* Q2 için pdf, cdf, map */
    float pdf[256];
    float cdf[256];
    uint8_t map[256];

    /* Median için pencere */
    uint8_t window[9];

    /* ---- Başlangıç görüntüsünü kopyala ---- */
    memcpy(img, gImageGray, IMG_N);

    /**************************************************************************
     * Q1) HISTOGRAM HESAPLAMA
     **************************************************************************/
    /* hist[] sıfırla */
    for (int i = 0; i < 256; i++)
        hist[i] = 0;

    /* Histogram hesapla */
    for (int i = 0; i < IMG_N; i++)
    {
        uint8_t val = img[i];
        hist[val]++;
    }

    /* (İstersen debug / printf ile bazı değerleri izle) */
    printf("Q1 - Histogram (ilk 10 seviye):\n");
    for (int i = 0; i < 10; i++)
        printf("hist[%d] = %lu\n", i, (unsigned long)hist[i]);

    /**************************************************************************
     * Q2) HISTOGRAM EQUALIZATION
     **************************************************************************/
    {
        /* PDF ve CDF hesapla */
        uint32_t size = IMG_N;

        for (int i = 0; i < 256; i++)
            pdf[i] = (float)hist[i] / (float)size;

        cdf[0] = pdf[0];
        for (int i = 1; i < 256; i++)
            cdf[i] = cdf[i - 1] + pdf[i];

        /* Mapping tablosu oluştur */
        for (int i = 0; i < 256; i++)
        {
            float s = cdf[i] * 255.0f;
            if (s < 0.0f)   s = 0.0f;
            if (s > 255.0f) s = 255.0f;
            map[i] = (uint8_t)(s + 0.5f);   // yuvarla
        }

        /* Görüntüye uygula (in-place) */
        for (int i = 0; i < IMG_N; i++)
        {
            img[i] = map[ img[i] ];
        }

        /* Equalization sonrası histogram */
        for (int i = 0; i < 256; i++)
            histEq[i] = 0;

        for (int i = 0; i < IMG_N; i++)
        {
            uint8_t val = img[i];
            histEq[val]++;
        }

        printf("\nQ2 - Equalization sonrasi histogram (ilk 10 seviye):\n");
        for (int i = 0; i < 10; i++)
            printf("histEq[%d] = %lu\n", i, (unsigned long)histEq[i]);
    }

    /**************************************************************************
     * Q3) 2D CONVOLUTION - LOW PASS & HIGH PASS
     **************************************************************************/

    /* LOW-PASS (3x3 averaging, normalize: /9) */
    {
        const int lowPass[3][3] = {
            {1, 1, 1},
            {1, 1, 1},
            {1, 1, 1}
        };
        int kernelSum = 9;

        /* tmp'yi sıfırla */
        memset(tmp, 0, IMG_N);

        for (int y = 1; y < (int)IMG_H - 1; y++)
        {
            for (int x = 1; x < (int)IMG_W - 1; x++)
            {
                int sum = 0;

                for (int ky = -1; ky <= 1; ky++)
                {
                    for (int kx = -1; kx <= 1; kx++)
                    {
                        uint8_t pixel = img[(y + ky) * IMG_W + (x + kx)];
                        sum += pixel * lowPass[ky + 1][kx + 1];
                    }
                }

                sum /= kernelSum;  // normalize

                if (sum < 0)   sum = 0;
                if (sum > 255) sum = 255;

                tmp[y * IMG_W + x] = (uint8_t)sum;
            }
        }

        printf("\nQ3-b - Low-pass sonrasi bazı pikseller:\n");
        for (int i = 0; i < 10; i++)
            printf("tmp[%d] = %u\n", i, tmp[i]);
    }

    /* HIGH-PASS (edge detection, normalize yok) */
    {
        const int highPass[3][3] = {
            {-1, -1, -1},
            {-1,  8, -1},
            {-1, -1, -1}
        };
        int kernelSum = 0;   // normalize yok

        /* tmp'yi sıfırla */
        memset(tmp, 0, IMG_N);

        for (int y = 1; y < (int)IMG_H - 1; y++)
        {
            for (int x = 1; x < (int)IMG_W - 1; x++)
            {
                int sum = 0;

                for (int ky = -1; ky <= 1; ky++)
                {
                    for (int kx = -1; kx <= 1; kx++)
                    {
                        uint8_t pixel = img[(y + ky) * IMG_W + (x + kx)];
                        sum += pixel * highPass[ky + 1][kx + 1];
                    }
                }

                if (sum < 0)   sum = 0;
                if (sum > 255) sum = 255;

                tmp[y * IMG_W + x] = (uint8_t)sum;
            }
        }

        printf("\nQ3-c - High-pass sonrasi bazı pikseller:\n");
        for (int i = 0; i < 10; i++)
            printf("tmp[%d] = %u\n", i, tmp[i]);
    }

    /**************************************************************************
     * Q4) MEDIAN FILTER (3x3)
     **************************************************************************/
    {
        /* Median input olarak equalized img'yi kullanıyoruz */
        memset(tmp, 0, IMG_N);

        for (int y = 1; y < (int)IMG_H - 1; y++)
        {
            for (int x = 1; x < (int)IMG_W - 1; x++)
            {
                int k = 0;
                for (int ky = -1; ky <= 1; ky++)
                {
                    for (int kx = -1; kx <= 1; kx++)
                    {
                        window[k++] = img[(y + ky) * IMG_W + (x + kx)];
                    }
                }

                /* 9 elemanlı window'u sırala (basit bubble sort) */
                for (int i = 0; i < 9; i++)
                {
                    for (int j = i + 1; j < 9; j++)
                    {
                        if (window[j] < window[i])
                        {
                            uint8_t tmpVal = window[i];
                            window[i] = window[j];
                            window[j] = tmpVal;
                        }
                    }
                }

                /* Ortadaki eleman median (index 4) */
                tmp[y * IMG_W + x] = window[4];
            }
        }

        printf("\nQ4 - Median filter sonrasi bazı pikseller:\n");
        for (int i = 0; i < 10; i++)
            printf("tmp[%d] = %u\n", i, tmp[i]);
    }
    while(1)

    /* Sonsuz döngü yok, PC’de test için program sonlanabilir.
       STM32'de olsan while(1) koyarsın. */
    return 0;
}