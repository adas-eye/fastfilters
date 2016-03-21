#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include "fastfilters.hxx"

namespace py = pybind11;

static py::array_t<float> iir_filter(fastfilters::iir::Coefficients &coefs, py::array_t<float> input)
{
    py::buffer_info info_in = input.request();
    const unsigned int ndim = (unsigned int)info_in.ndim;

    if (info_in.ndim <= 0)
        throw std::runtime_error("Number of dimensions must be > 0");

    auto result = py::array(py::buffer_info(nullptr, sizeof(float), py::format_descriptor<float>::value(), ndim,
                                            info_in.shape, info_in.strides));
    py::buffer_info info_out = result.request();

    const float *inptr = (float *)info_in.ptr;
    float *outptr = (float *)info_out.ptr;

    unsigned int n_times = 1;
    for (unsigned int i = 0; i < ndim - 1; ++i)
        n_times *= info_in.shape[i];

    if (fastfilters::detail::cpu_has_avx2())
        fastfilters::iir::convolve_iir_inner_single_avx(inptr, info_in.shape[ndim - 1], n_times, outptr, coefs);
    else
        fastfilters::iir::convolve_iir_inner_single(inptr, info_in.shape[ndim - 1], n_times, outptr, coefs);

    for (unsigned int i = 0; i < ndim - 1; ++i) {
        n_times = 1;
        for (unsigned int j = 0; j < ndim; ++j)
            if (j != i)
                n_times *= info_in.shape[j];

        if (fastfilters::detail::cpu_has_avx2())
            fastfilters::iir::convolve_iir_outer_single_avx(outptr, info_in.shape[i], n_times, outptr, coefs);
        else
            fastfilters::iir::convolve_iir_outer_single(outptr, info_in.shape[i], n_times, outptr, coefs, n_times);
    }

    return result;
}

PYBIND11_PLUGIN(pyfastfilters)
{
    py::module m("pyfastfilters", "fast gaussian kernel and derivative filters");

    py::class_<fastfilters::iir::Coefficients>(m, "IIRCoefficients")
        .def(py::init<const double, const unsigned int>())
        .def("__repr__", [](const fastfilters::iir::Coefficients &a) {
            std::ostringstream oss;
            oss << "<pyfastfilters.IIRCoefficients with sigma = " << a.sigma << " and order = " << a.order << ">";

            return oss.str();
        })
        .def_readonly("sigma", &fastfilters::iir::Coefficients::sigma)
        .def_readonly("order", &fastfilters::iir::Coefficients::order);

    m.def("iir_filter", &iir_filter, "apply IIR filter to all dimensions of array and return result.");

    m.def("cpu_has_avx2", &fastfilters::detail::cpu_has_avx2);

    return m.ptr();
}