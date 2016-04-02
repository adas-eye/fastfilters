// fastfilters
// Copyright (c) 2016 Sven Peter
// sven.peter@iwr.uni-heidelberg.de or mail@svenpeter.me
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
// Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
#ifndef FASTFILTERS_HXX
#define FASTFILTERS_HXX

#include <array>
#include <vector>

#if defined(_MSC_VER)
#if defined(FASTFILTERS_SHARED_LIBRARY)
#define FASTFILTERS_API_EXPORT __declspec(dllexport)
#elif defined(FASTFILTERS_STATIC_LIBRARY)
#define FASTFILTERS_API_EXPORT
#else
#define FASTFILTERS_API_EXPORT __declspec(dllimport)
#endif
#else
#define FASTFILTERS_API_EXPORT
#endif

namespace fastfilters
{

namespace detail
{
FASTFILTERS_API_EXPORT bool cpu_has_avx2();
FASTFILTERS_API_EXPORT bool cpu_has_avx();
FASTFILTERS_API_EXPORT bool cpu_has_avx_fma();

FASTFILTERS_API_EXPORT bool cpu_enable_avx2(bool enable);
FASTFILTERS_API_EXPORT bool cpu_enable_avx(bool enable);
FASTFILTERS_API_EXPORT bool cpu_enable_avx_fma(bool enable);
}

namespace fir
{

FASTFILTERS_API_EXPORT bool make_gaussian(double stddev, unsigned order, unsigned n_coefs, std::array<float, 13> &v);

struct FASTFILTERS_API_EXPORT Kernel
{
  private:
    void computeFull()
    {
        for (std::size_t idx = 0; idx < len(); ++idx) {
            float v;

            if (idx == half_len())
                v = coefs[0];
            else if (idx < half_len()) {
                if (is_symmetric)
                    v = coefs[half_len() - idx];
                else
                    v = -coefs[half_len() - idx];
            } else
                v = coefs[idx - half_len()];
            coefs2[idx] = v;
        }
    }

  public:
    const bool is_symmetric;
    std::array<float, 13> coefs;
    std::array<float, 27> coefs2;
    const std::size_t size;

    inline Kernel(bool is_symmetric, const std::vector<float> &coefs_) : is_symmetric(is_symmetric), size(coefs_.size())
    {
        for (unsigned idx = 0; idx < size; ++idx)
            coefs[idx] = coefs_[idx];

        computeFull();
    }

    inline Kernel(double stddev, unsigned order) : is_symmetric((order & 1) == 0), size((unsigned int)(stddev * 4.0))
    {
        std::vector<float> c;
        unsigned n_coefs = (unsigned int)(stddev * 4.0);

        if (n_coefs > 12)
            throw std::logic_error("stddev is too large.");

        if (!make_gaussian(stddev, order, n_coefs, coefs))
            throw std::logic_error("make_gaussian failed.");

        computeFull();
    }

    inline float operator[](std::size_t idx) const
    {
        return coefs2[idx];
    };

    inline std::size_t len() const
    {
        return 2 * size - 1;
    }
    inline std::size_t half_len() const
    {
        return size - 1;
    }
};

FASTFILTERS_API_EXPORT void convolve_fir(const float *input, const unsigned int pixel_stride,
                                         const unsigned int pixel_n, const unsigned int dim_stride,
                                         const unsigned int dim, float *output, Kernel &kernel);
}

namespace iir
{

struct FASTFILTERS_API_EXPORT Coefficients
{
    std::array<float, 4> n_causal;
    std::array<float, 4> n_anticausal;
    std::array<float, 4> d;
    double sigma;
    unsigned order;
    unsigned n_border;

    Coefficients(const double sigma, const unsigned order);
};

FASTFILTERS_API_EXPORT void convolve_iir_inner_single_noavx(const float *input, const unsigned int n_pixels,
                                                            const unsigned n_times, float *output,
                                                            const Coefficients &coefs);

FASTFILTERS_API_EXPORT void convolve_iir_outer_single_noavx(const float *input, const unsigned int n_pixels,
                                                            const unsigned n_times, float *output,
                                                            const Coefficients &coefs, const unsigned stride);

FASTFILTERS_API_EXPORT void convolve_iir_inner_single_avx(const float *input, const unsigned int n_pixels,
                                                          const unsigned n_times, float *output,
                                                          const Coefficients &coefs);

FASTFILTERS_API_EXPORT void convolve_iir_outer_single_avx(const float *input, const unsigned int n_pixels,
                                                          const unsigned n_times, float *output,
                                                          const Coefficients &coefs);

FASTFILTERS_API_EXPORT void convolve_iir_inner_single(const float *input, const unsigned int n_pixels,
                                                      const unsigned n_times, float *output, const Coefficients &coefs);

FASTFILTERS_API_EXPORT void convolve_iir_outer_single(const float *input, const unsigned int n_pixels,
                                                      const unsigned n_times, float *output, const Coefficients &coefs);

} // namespace iir

} // namespace fastfilters

#endif