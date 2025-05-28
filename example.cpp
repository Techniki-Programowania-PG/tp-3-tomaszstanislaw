#include <pybind11/pybind11.h>
#include <matplot/matplot.h>
#include <cmath>
#include <vector>
#include <string>
#include <complex>
#include <iostream>

namespace py = pybind11;
using namespace matplot;

using Complex = std::complex<double>;
const double PI = 3.14159265358979323846;


std::vector<Complex> dft(const std::vector<double>& in) {
    size_t N = in.size();
    std::vector<Complex> out(N);
    for (size_t k = 0; k < N; ++k) {
        for (size_t n = 0; n < N; ++n) {
            double angle = -2 * PI * k * n / N;
            out[k] += in[n] * Complex(std::cos(angle), std::sin(angle));
        }
    }
    return out;
}


std::vector<double> idft(const std::vector<Complex>& in) {
    size_t N = in.size();
    std::vector<double> out(N);
    for (size_t n = 0; n < N; ++n) {
        Complex sum = 0;
        for (size_t k = 0; k < N; ++k) {
            double angle = 2 * PI * k * n / N;
            sum += in[k] * Complex(std::cos(angle), std::sin(angle));
        }
        out[n] = sum.real() / N;
    }
    return out;
}

//dolnoprzepustowy 1D 
std::vector<double> filter_1d(const std::vector<double>& signal, int window_size = 5) {
    int N = signal.size();
    std::vector<double> filtered(N, 0.0);
    int half = window_size / 2;

    for (int i = 0; i < N; ++i) {
        double sum = 0;
        int count = 0;
        for (int j = i - half; j <= i + half; ++j) {
            if (j >= 0 && j < N) {
                sum += signal[j];
                count++;
            }
        }
        filtered[i] = sum / count;
    }

 
    std::vector<double> t(N);
    for (int i = 0; i < N; ++i) t[i] = i;

    plot(t, signal)->line_width(1).color("blue").display_name("Oryginalny sygnał");
    hold(on);
    plot(t, filtered)->line_width(2).color("red").display_name("Filtrowany sygnał");
    matplot::legend();
    title("Filtracja 1D (średnia ruchoma)");
    xlabel("Próbka");
    ylabel("Amplituda");
    grid(on);
    show();

    return filtered;
}


std::vector<std::vector<double>> filter_2d(const std::vector<std::vector<double>>& signal, int window_size = 3) {
    int M = signal.size();
    int N = signal[0].size();
    std::vector<std::vector<double>> filtered(M, std::vector<double>(N, 0.0));
    int half = window_size / 2;

    for (int x = 0; x < M; ++x) {
        for (int y = 0; y < N; ++y) {
            double sum = 0;
            int count = 0;
            for (int i = x - half; i <= x + half; ++i) {
                for (int j = y - half; j <= y + half; ++j) {
                    if (i >= 0 && i < M && j >= 0 && j < N) {
                        sum += signal[i][j];
                        count++;
                    }
                }
            }
            filtered[x][y] = sum / count;
        }
    }

// tu jest heatmapa
    imagesc(filtered);
    colorbar();
    title("Filtracja 2D (średnia ruchoma)");
    show();

    return filtered;
}

// w tej funkcji generowany jest sygnal - popraw petle for
std::vector<double> generate_signal_1d(const std::string& wave_type, double freq, double sample_rate,
                                       float time_start, float time_end) {
    float duration = time_end - time_start;
    int N = static_cast<int>(sample_rate * duration);
    std::vector<double> y(N);

    for (int i = 0; i < N; ++i) {
        double t = time_start + i / sample_rate;
        if (wave_type == "sin") {
            y[i] = std::sin(2 * PI * freq * t);
        } else if (wave_type == "cos") {
            y[i] = std::cos(2 * PI * freq * t);
        } else if (wave_type == "square") {
            y[i] = std::sin(2 * PI * freq * t) >= 0 ? 1.0 : -1.0;
        } else if (wave_type == "sawtooth") {
            y[i] = 2.0 * (t * freq - std::floor(0.5 + t * freq));
        } else {
            y[i] = 0.0;
        }
    }
    return y;
}


std::vector<std::vector<double>> generate_signal_2d(const std::string& wave_type, double freq, int M = 50) {
    std::vector<std::vector<double>> mat(M, std::vector<double>(M));
    for (int x = 0; x < M; ++x) {
        for (int y = 0; y < M; ++y) {
            double val = 0;
            double norm_x = x / (double)M;
            double norm_y = y / (double)M;
            if (wave_type == "sin") {
                val = std::sin(2 * PI * freq * norm_x) * std::sin(2 * PI * freq * norm_y);
            } else if (wave_type == "cos") {
                val = std::cos(2 * PI * freq * norm_x) * std::cos(2 * PI * freq * norm_y);
            } else if (wave_type == "square") {
                val = (std::sin(2 * PI * freq * norm_x) >= 0 ? 1.0 : -1.0) * (std::sin(2 * PI * freq * norm_y) >= 0 ? 1.0 : -1.0);
            } else if (wave_type == "sawtooth") {
                val = 2.0 * (norm_x * freq - std::floor(0.5 + norm_x * freq)) * 2.0 * (norm_y * freq - std::floor(0.5 + norm_y * freq));
            }
            mat[x][y] = val;
        }
    }
    return mat;
}

// tu znajduje piki w zwyklym sygnale jak jest cos innego to nie ma piku
std::vector<std::pair<int, double>> find_peaks(const std::vector<double>& signal) {
    std::vector<std::pair<int, double>> peaks;
    int N = signal.size();
    for (int i = 1; i < N - 1; ++i) {
        if (signal[i] > signal[i - 1] && signal[i] > signal[i + 1]) {
            peaks.emplace_back(i, signal[i]);
        }
    }
    return peaks;
}


void generate_and_plot_matplot(const std::string& wave_type, double freq, double sample_rate,
                               float time_start, float time_end,
                               const std::string& action = "") {
    if (action == "dft" || action == "idft") {
     
        auto signal = generate_signal_1d(wave_type, freq, sample_rate, time_start, time_end);
        int N = signal.size();
        std::vector<double> t(N);
        for (int i = 0; i < N; ++i) t[i] = time_start + i / sample_rate;

        if (action == "dft") {
            auto spectrum = dft(signal);
            std::vector<double> amplitude(spectrum.size());
            for (size_t i = 0; i < spectrum.size(); ++i)
                amplitude[i] = std::abs(spectrum[i]);

            plot(t, amplitude);
            title("DFT 1D amplituda");
            xlabel("Czas [s]");
            ylabel("Amplituda");
            grid(on);
            show();
        } else {
            auto spectrum = dft(signal);
            auto rec = idft(spectrum);
            plot(t, rec);
            title("IDFT 1D rekonstruowany sygnał");
            xlabel("Czas [s]");
            ylabel("Amplituda");
            grid(on);
            show();
        }
    } else if (action == "filter1d") {
        auto signal = generate_signal_1d(wave_type, freq, sample_rate, time_start, time_end);
        filter_1d(signal);
    } else if (action == "filter2d") {
        auto signal2d = generate_signal_2d(wave_type, freq);
        filter_2d(signal2d);
    } else {
      
        auto signal = generate_signal_1d(wave_type, freq, sample_rate, time_start, time_end);
        int N = signal.size();
        std::vector<double> t(N);
        for (int i = 0; i < N; ++i) t[i] = time_start + i / sample_rate;

        hold(on);
        auto p = plot(t, signal);
        p->line_width(2);
        title("Fala: " + wave_type);
        xlabel("Czas [s]");
        ylabel("Amplituda");
        grid(on);

      
        auto peaks = find_peaks(signal);
        std::vector<double> peak_times, peak_vals;
        for (auto& p : peaks) {
            peak_times.push_back(t[p.first]);
            peak_vals.push_back(p.second);
        }

        auto s = scatter(peak_times, peak_vals);
        s->marker("o");
        s->marker_face(true);
        s->color("red");
        s->marker_size(10);

        show();

        std::cout << "Wykryto " << peaks.size() << " pików:\n";
        for (auto& p : peaks) {
            std::cout << "czas: " << t[p.first] << ", wartosc: " << p.second << "\n";
        }
    }
}

PYBIND11_MODULE(example, m) {
    m.def("dft", &dft, "Transformacja Fouriera 1D");
    m.def("idft", &idft, "Odwrotna transformacja Fouriera 1D");
    m.def("filter_1d", &filter_1d, py::arg("signal"), py::arg("window_size")=5, "Filtracja 1D (średnia ruchoma)");
    m.def("filter_2d", &filter_2d, py::arg("signal"), py::arg("window_size")=3, "Filtracja 2D (średnia ruchoma)");
    m.def("generate_and_plot_matplot", &generate_and_plot_matplot,
          py::arg("wave_type"),
          py::arg("freq"),
          py::arg("sample_rate"),
          py::arg("time_start"),
          py::arg("time_end"),
          py::arg("action") = "",
          "Generuj falę i rysuj: dft, idft, filter1d, filter2d lub pokaz sygnał");
}
