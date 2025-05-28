import example

def main():
    wave = input("Typ fali (sin, cos, square, sawtooth): ").strip().lower()
    if wave not in ("sin", "cos", "square", "sawtooth"):
        print("Nieznany typ fali. Ustawiam 'sin'.")
        wave = "sin"

    try:
        freq = float(input("Częstotliwość [Hz]: "))
    except ValueError:
        print("Niepoprawna częstotliwość. Ustawiam 5 Hz.")
        freq = 5.0

    try:
        sample_rate = int(input("Ilość próbek na sekundę: "))
    except ValueError:
        print("Niepoprawna liczba próbek. Ustawiam 500.")
        sample_rate = 500

    try:
        time_start = float(input("Czas początkowy [s]: "))
        time_end = float(input("Czas końcowy [s]: "))
        if time_end <= time_start:
            raise ValueError
    except ValueError:
        print("Niepoprawny zakres czasu. Ustawiam 0 do 1 sekundy.")
        time_start = 0.0
        time_end = 1.0

    transform = input("Transformacja (brak, dft, idft, filter1d, filter2d): ").strip().lower()

    valid_transforms = ("", "brak", "dft", "idft", "filter1d", "filter2d")

    if transform in ("", "brak"):
        transform = ""
    elif transform not in valid_transforms:
        print(f"Nieznana transformacja '{transform}'. Ustawiam brak transformacji.")
        transform = ""

    # w 2d dalem ze dziala tylko sinus
    #git jest
    if transform == "filter2d" and wave != "sin":
        print("Filtr 2D działa tylko dla fali typu 'sin'. Zmieniam na 'sin'.")
        wave = "sin"


    if transform != "":
        print("Uwaga: piki wykrywane są tylko dla oryginalnego sygnału (bez transformacji).")

    example.generate_and_plot_matplot(wave, freq, sample_rate, time_start, time_end, transform)

if __name__ == "__main__":
    main()
