#include <cstddef>
#include <cmath>

template <typename FLOAT>
void normalize(FLOAT *data, size_t sz)
{
    FLOAT avg = 0;

    for (size_t i = 0; i < sz; ++i)
    {
        avg += data[i];
    }
    avg /= sz;
    for (size_t i = 0; i < sz; i++)
    {
        data[i] -= avg;
    }
    FLOAT sum = 0;
    for (size_t i = 0; i < sz; ++i)
    {
        const FLOAT x = data[i];
        sum += x * x;
    }
    const FLOAT dev = sqrt(sum / sz);
    if (dev != 0.0)
    {
        for (size_t i = 0; i < sz; ++i)
        {
            data[i] /= dev;
        }
    }
}
