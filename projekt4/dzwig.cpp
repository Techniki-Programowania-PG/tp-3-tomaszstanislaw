#include <windows.h>
#include <gdiplus.h>
#include <vector>
using namespace Gdiplus;
#pragma comment (lib,"Gdiplus.lib")

HINSTANCE hInst;
ULONG_PTR gdiplusToken;
const wchar_t* szWindowClass = L"DzwigWindow";
const wchar_t* szTitle = L"Dzwig - GDI+ Application";
UINT_PTR timerId = 1;

const int POCZATEK_SZYNY = 240;
const int POZIOM_PODLOGI = 600; // 720 - ponizej ramki - wysokosc klocka

int dzwigPozycjaX = 240;
int dzwigPozycjaY = 310;
int nowyDzwigX = dzwigPozycjaX;
int nowyDzwigY = dzwigPozycjaY;
const int DZWIG_PREDKOSC_X = 6;
const int DZWIG_PREDKOSC_Y = 6;
const int KLOCKI_PREDKOSC_Y = 3;
const int ROZMIAR_KLOCKA = 60;
const int ROZMIAR_OKRAGU_X = 60;
const int ROZMIAR_OKRAGU_Y = 60;
const int ROZMIAR_TROJKATA = 60;
bool ruchLewo = false;
bool ruchPrawo = false;
bool ruchGora = false;
bool ruchDol = false;

bool brakMiejsca = false;
bool bladMagnes = false;
bool bladTryb = false;
bool bladMasa = false;
bool czyMagnesDziala = false;
int indeksPrzyczepionegoKwadratu = -1;
int indeksPrzyczepionegoTrojkata = -1;
int indeksPrzyczepionegoOkragu = -1;
int masaDodawanegoKlocka = 6;

DWORD poczatekOstrzezenia = 0;
DWORD poczatekOstrzezeniaBHP = 0;
DWORD poczatekOstrzezeniaTryb = 0;
DWORD poczatekOstrzezeniaMasa = 0;
const DWORD CZAS_OSTRZEZENIA = 1500;

enum TypTrybu {
    TRYB_KWADRATY = 0,
    TRYB_OKREGI = 1,
    TRYB_TROJKATY = 2
};

TypTrybu aktualnyTryb = TRYB_KWADRATY;

struct Przycisk {
    int x, y, szer, wys;
    const wchar_t* napis;
    bool czyWcisniety;
};

struct Kwadrat {
    int x, y;
    bool czyMagnes;
    bool spadanie = false;
    int masa;
};

struct Okrag {
    int x, y;
    bool czyMagnes;
    bool spadanie = false;
    int masa;
};

struct Trojkat {
    int x, y;
    bool czyMagnes;
    bool spadanie = false;
    int masa;
};

std::vector<Kwadrat> kwadraty;
std::vector<Okrag> okregi;
std::vector<Trojkat> trojkaty;
Przycisk przyciskDodajKwadrat = { 10, 5, 300, 50, L"Dodaj kwadratowy klocek", false };
Przycisk przyciskZmienTrybKwadraty = { 1150, 60, 100, 50, L"Tryb kwadraty", false };
Przycisk przyciskDodajOkrag = { 400, 5, 300, 50, L"Dodaj okragly klocek", false };
Przycisk przyciskZmienTrybOkregi = { 1150, 120, 100, 50, L"Tryb okregi", false };
Przycisk przyciskDodajTrojkat = { 800, 5, 300, 50, L"Dodaj Trojkatny klocek", false };
Przycisk przyciskZmienTrybTrojkaty = { 1150, 180, 100, 50, L"Tryb trojkaty", false };
Przycisk przyciskWlaczMagnes = { 10, 665, 300, 50, L"Wlacz magnes", false };
Przycisk przyciskUsunKlocki = { 10, 60, 120, 50, L"Usun klocki", false };
Przycisk przyciskMinus = { 10, 120, 55, 50, L"-", false };
Przycisk przyciskPlus = { 70, 120, 55, 50, L"+", false };

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

bool sprawdzenieKolizji(int x1, int y1, int szer1, int wys1, int x2, int y2, int szer2, int wys2) {
    return !(x1 + szer1 <= x2 || x1 >= x2 + szer2 || y1 + wys1 <= y2 || y1 >= y2 + wys2);
}

void usuwanieKlockow() {
    kwadraty.clear();
    okregi.clear();
    trojkaty.clear();

    indeksPrzyczepionegoKwadratu = -1;
    indeksPrzyczepionegoOkragu = -1;
    indeksPrzyczepionegoTrojkata = -1;

    czyMagnesDziala = false;
}

int znajdzPoziomPodlogi(int klocekIndeks, int x) {
    int podloga = POZIOM_PODLOGI;

    for (int i = 0; i < kwadraty.size(); i++) {
        if (i == klocekIndeks) continue;
        const auto& klocek = kwadraty[i];
        if (x < klocek.x + ROZMIAR_KLOCKA && x + ROZMIAR_KLOCKA > klocek.x) {
            int poziom = klocek.y - ROZMIAR_KLOCKA;
            if (poziom < podloga) podloga = poziom;
        }
    }

    for (int i = 0; i < okregi.size(); i++) {
        const auto& klocek = okregi[i];
        if (x < klocek.x + ROZMIAR_OKRAGU_X && x + ROZMIAR_KLOCKA > klocek.x) {
            int poziom = klocek.y - ROZMIAR_KLOCKA;
            if (poziom < podloga) podloga = poziom;
        }
    }

    for (int i = 0; i < trojkaty.size(); i++) {
        const auto& klocek = trojkaty[i];
        if (x < klocek.x + ROZMIAR_KLOCKA && x + ROZMIAR_KLOCKA > klocek.x) {
            int poziom = klocek.y - ROZMIAR_KLOCKA;
            if (poziom < podloga) podloga = poziom;
        }
    }

    return podloga;
}

bool czyMozliweSpadanie(int index) {
    bool czyKwadrat = index < (int)kwadraty.size();
    bool czyOkrag = (!czyKwadrat && index < (int)kwadraty.size() + (int)okregi.size());
    bool czyTrojkat = !czyKwadrat && !czyOkrag;

    int x, y, szer, wys;

    if (czyKwadrat) {
        const auto& k = kwadraty[index];
        x = k.x;
        y = k.y + KLOCKI_PREDKOSC_Y;
        szer = ROZMIAR_KLOCKA;
        wys = ROZMIAR_KLOCKA;
    }
    else if (czyOkrag) {
        int iOkrag = index - (int)kwadraty.size();
        const auto& o = okregi[iOkrag];
        x = o.x;
        y = o.y + KLOCKI_PREDKOSC_Y;
        szer = ROZMIAR_OKRAGU_X;
        wys = ROZMIAR_OKRAGU_Y;
    }
    else {
        int iTrojkat = index - (int)kwadraty.size() - (int)okregi.size();
        const auto& t = trojkaty[iTrojkat];
        x = t.x;
        y = t.y + KLOCKI_PREDKOSC_Y;
        szer = ROZMIAR_KLOCKA;
        wys = ROZMIAR_KLOCKA;
    }

    for (int i = 0; i < (int)kwadraty.size(); ++i) {
        if (czyKwadrat && i == index) continue;
        if (sprawdzenieKolizji(x, y, szer, wys,
            kwadraty[i].x, kwadraty[i].y, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA)) {
            return false;
        }
    }

    for (int i = 0; i < (int)okregi.size(); ++i) {
        if (czyOkrag && i == index - (int)kwadraty.size()) continue;
        if (sprawdzenieKolizji(x, y, szer, wys,
            okregi[i].x, okregi[i].y, ROZMIAR_OKRAGU_X, ROZMIAR_OKRAGU_Y)) {
            return false;
        }
    }

    for (int i = 0; i < (int)trojkaty.size(); ++i) {
        if (czyTrojkat && i == index - (int)kwadraty.size() - (int)okregi.size()) continue;
        if (sprawdzenieKolizji(x, y, szer, wys,
            trojkaty[i].x, trojkaty[i].y, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA)) {
            return false;
        }
    }

    if (y + wys > POZIOM_PODLOGI) return false;

    return true;
}

bool czyMozliwyRuchMagnes(int nowyX, int nowyY) {
    int magnesX = nowyX - 15;
    int magnesY = nowyY;

    for (int i = 0; i < kwadraty.size(); i++) {
        if (i == indeksPrzyczepionegoKwadratu) continue;
        const auto& kwadrat = kwadraty[i];
        if (sprawdzenieKolizji(magnesX, magnesY, 50, 15,
            kwadrat.x, kwadrat.y, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA)) return false;
    }

    for (int i = 0; i < okregi.size(); i++) {
        if (i == indeksPrzyczepionegoOkragu) continue;
        const auto& okrag = okregi[i];
        if (sprawdzenieKolizji(magnesX, magnesY, 50, 15,
            okrag.x, okrag.y, ROZMIAR_OKRAGU_X, ROZMIAR_OKRAGU_Y)) return false;
    }

    for (int i = 0; i < trojkaty.size(); i++) {
        if (i == indeksPrzyczepionegoTrojkata) continue;
        const auto& trojkat = trojkaty[i];
        if (sprawdzenieKolizji(magnesX, magnesY, 50, 15,
            trojkat.x, trojkat.y, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA)) return false;
    }

    int trzymanyX = nowyX - 20;
    int trzymanyY = nowyY + 15;

    if (indeksPrzyczepionegoKwadratu != -1) {
        for (int i = 0; i < kwadraty.size(); i++) {
            if (i == indeksPrzyczepionegoKwadratu) continue;
            const auto& kwadrat = kwadraty[i];
            if (sprawdzenieKolizji(trzymanyX, trzymanyY, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA,
                kwadrat.x, kwadrat.y, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA)) return false;
        }
        for (const auto& okrag : okregi) {
            if (sprawdzenieKolizji(trzymanyX, trzymanyY, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA,
                okrag.x, okrag.y, ROZMIAR_OKRAGU_X, ROZMIAR_OKRAGU_Y)) return false;
        }
        for (const auto& trojkat : trojkaty) {
            if (sprawdzenieKolizji(trzymanyX, trzymanyY, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA,
                trojkat.x, trojkat.y, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA)) return false;
        }
    }

    if (indeksPrzyczepionegoOkragu != -1) {
        for (int i = 0; i < okregi.size(); i++) {
            if (i == indeksPrzyczepionegoOkragu) continue;
            const auto& okrag = okregi[i];
            if (sprawdzenieKolizji(trzymanyX, trzymanyY, ROZMIAR_OKRAGU_X, ROZMIAR_OKRAGU_Y,
                okrag.x, okrag.y, ROZMIAR_OKRAGU_X, ROZMIAR_OKRAGU_Y)) return false;
        }
        for (const auto& kwadrat : kwadraty) {
            if (sprawdzenieKolizji(trzymanyX, trzymanyY, ROZMIAR_OKRAGU_X, ROZMIAR_OKRAGU_Y,
                kwadrat.x, kwadrat.y, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA)) return false;
        }
        for (const auto& trojkat : trojkaty) {
            if (sprawdzenieKolizji(trzymanyX, trzymanyY, ROZMIAR_OKRAGU_X, ROZMIAR_OKRAGU_Y,
                trojkat.x, trojkat.y, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA)) return false;
        }
    }

    if (indeksPrzyczepionegoTrojkata != -1) {
        for (int i = 0; i < trojkaty.size(); i++) {
            if (i == indeksPrzyczepionegoTrojkata) continue;
            const auto& trojkat = trojkaty[i];
            if (sprawdzenieKolizji(trzymanyX, trzymanyY, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA,
                trojkat.x, trojkat.y, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA)) return false;
        }
        for (const auto& kwadrat : kwadraty) {
            if (sprawdzenieKolizji(trzymanyX, trzymanyY, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA,
                kwadrat.x, kwadrat.y, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA)) return false;
        }
        for (const auto& okrag : okregi) {
            if (sprawdzenieKolizji(trzymanyX, trzymanyY, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA,
                okrag.x, okrag.y, ROZMIAR_OKRAGU_X, ROZMIAR_OKRAGU_Y)) return false;
        }
    }

    return true;
}

bool czyMozliweWylaczenieMagnesuDlaKwadratu(int startX, int startY, int indeksKwadratu) {
    for (int y = startY; y <= POZIOM_PODLOGI; y += KLOCKI_PREDKOSC_Y) {

        for (int i = 0; i < kwadraty.size(); i++) {
            if (i == indeksKwadratu) continue;
            const auto& kwadrat = kwadraty[i];
            if (sprawdzenieKolizji(startX, y, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA,
                kwadrat.x, kwadrat.y, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA)) {
                if (aktualnyTryb == TRYB_KWADRATY) {
                    return true;
                }
                return false;
            }
        }

        for (const auto& okrag : okregi) {
            if (sprawdzenieKolizji(startX, y, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA,
                okrag.x, okrag.y, ROZMIAR_OKRAGU_X, ROZMIAR_OKRAGU_Y)) {
                if (aktualnyTryb == TRYB_KWADRATY) {
                    return false;
                }
                return true;
            }
        }

        for (const auto& trojkat : trojkaty) {
            if (sprawdzenieKolizji(startX, y, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA,
                trojkat.x, trojkat.y, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA)) {
                if (aktualnyTryb == TRYB_KWADRATY) {
                    return false;
                }
                return true;
            }
        }
    }

    return true;
}

bool czyMozliweWylaczenieMagnesuDlaOkregu(int startX, int startY, int indeksOkregu) {
    for (int y = startY; y <= POZIOM_PODLOGI; y += KLOCKI_PREDKOSC_Y) {

        for (int i = 0; i < okregi.size(); i++) {
            if (i == indeksOkregu) continue;
            const auto& okrag = okregi[i];
            if (sprawdzenieKolizji(startX, y, ROZMIAR_OKRAGU_X, ROZMIAR_OKRAGU_Y,
                okrag.x, okrag.y, ROZMIAR_OKRAGU_X, ROZMIAR_OKRAGU_Y)) {
                if (aktualnyTryb == TRYB_OKREGI) {
                    return true;
                }
                return false;
            }
        }


        for (const auto& kwadrat : kwadraty) {
            if (sprawdzenieKolizji(startX, y, ROZMIAR_OKRAGU_X, ROZMIAR_OKRAGU_Y,
                kwadrat.x, kwadrat.y, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA)) {
                if (aktualnyTryb == TRYB_OKREGI) {
                    return false;
                }
                return true;
            }
        }


        for (const auto& trojkat : trojkaty) {
            if (sprawdzenieKolizji(startX, y, ROZMIAR_OKRAGU_X, ROZMIAR_OKRAGU_Y,
                trojkat.x, trojkat.y, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA)) {
                if (aktualnyTryb == TRYB_OKREGI) {
                    return false;
                }
                return true;
            }
        }
    }

    return true;
}

bool czyMozliweWylaczenieMagnesuDlaTrojkata(int startX, int startY, int indeksTrojkata) {
    for (int y = startY; y <= POZIOM_PODLOGI; y += KLOCKI_PREDKOSC_Y) {


        for (int i = 0; i < trojkaty.size(); i++) {
            if (i == indeksTrojkata) continue;
            const auto& trojkat = trojkaty[i];
            if (sprawdzenieKolizji(startX, y, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA,
                trojkat.x, trojkat.y, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA)) {
                if (aktualnyTryb == TRYB_TROJKATY) {
                    return true;
                }
                return false;
            }
        }


        for (const auto& kwadrat : kwadraty) {
            if (sprawdzenieKolizji(startX, y, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA,
                kwadrat.x, kwadrat.y, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA)) {
                if (aktualnyTryb == TRYB_TROJKATY) {
                    return false;
                }
                return true;
            }
        }


        for (const auto& okrag : okregi) {
            if (sprawdzenieKolizji(startX, y, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA,
                okrag.x, okrag.y, ROZMIAR_OKRAGU_X, ROZMIAR_OKRAGU_Y)) {
                if (aktualnyTryb == TRYB_TROJKATY) {
                    return false;
                }
                return true;
            }
        }
    }

    return true;
}

bool czyMoznaWylaczycMagnes() {
    if (nowyDzwigY <= 430) return false;


    int trzymanyX = dzwigPozycjaX - 20;
    int trzymanyY = dzwigPozycjaY + 15;

    if (indeksPrzyczepionegoKwadratu != -1) {
        return czyMozliweWylaczenieMagnesuDlaKwadratu(trzymanyX, trzymanyY, indeksPrzyczepionegoKwadratu);
    }
    else if (indeksPrzyczepionegoOkragu != -1) {
        return czyMozliweWylaczenieMagnesuDlaOkregu(trzymanyX, trzymanyY, indeksPrzyczepionegoOkragu);
    }
    else if (indeksPrzyczepionegoTrojkata != -1) {
        return czyMozliweWylaczenieMagnesuDlaTrojkata(trzymanyX, trzymanyY, indeksPrzyczepionegoTrojkata);
    }

    return true;
}

int sprawdzenieOkraguPodMagnesem() {
    int magnesSrodekX = dzwigPozycjaX + 10;
    int magnesSrodekY = dzwigPozycjaY + 15;

    for (int i = 0; i < okregi.size(); i++) {
        const auto& okrag = okregi[i];
        if (magnesSrodekX >= okrag.x - 5 && magnesSrodekX <= okrag.x + 65 &&
            magnesSrodekY >= okrag.y - 5 && magnesSrodekY <= okrag.y + 60)
            return i;
    }
    return -1;
}

int sprawdzenieKlockaPodMagnesem() {
    int magnesSrodekX = dzwigPozycjaX + 10;
    int magnesSrodekY = dzwigPozycjaY + 15;

    for (int i = 0; i < kwadraty.size(); i++) {
        const auto& kwadrat = kwadraty[i];
        if (magnesSrodekX >= kwadrat.x - 5 && magnesSrodekX <= kwadrat.x + 65 &&
            magnesSrodekY >= kwadrat.y - 5 && magnesSrodekY <= kwadrat.y + 60) return i;
    }
    return -1;
}

int sprawdzenieTrojkataPodMagnesem() {
    int magnesSrodekX = dzwigPozycjaX + 10;
    int magnesSrodekY = dzwigPozycjaY + 15;

    for (int i = 0; i < trojkaty.size(); i++) {
        const auto& trojkat = trojkaty[i];
        if (magnesSrodekX >= trojkat.x - 5 && magnesSrodekX <= trojkat.x + 65 &&
            magnesSrodekY >= trojkat.y - 5 && magnesSrodekY <= trojkat.y + 60)
            return i;
    }
    return -1;
}

bool czyMiejsceWolneKwadrat() {
    for (const auto& kwadrat : kwadraty) {
        if (kwadrat.x <= POCZATEK_SZYNY + 40) return false;
    }
    return true;
}

bool czyMiejsceWolneOkrag() {
    for (const auto& okrag : okregi) {
        if (okrag.x <= POCZATEK_SZYNY + 40) return false;
    }
    return true;
}

bool czyMiejsceWolneTrojkat() {
    for (const auto& trojkat : trojkaty) {
        if (trojkat.x <= POCZATEK_SZYNY + 40) return false;
    }
    return true;
}

void DodajKwadrat() {
    if (czyMiejsceWolneKwadrat() && czyMiejsceWolneOkrag() && czyMiejsceWolneTrojkat()) {
        Kwadrat nowyKwadrat;
        nowyKwadrat.x = POCZATEK_SZYNY - 20;
        nowyKwadrat.y = POZIOM_PODLOGI;
        nowyKwadrat.czyMagnes = false;
        nowyKwadrat.spadanie = false;
        nowyKwadrat.masa = masaDodawanegoKlocka;
        kwadraty.push_back(nowyKwadrat);
        brakMiejsca = false;
        return;
    }
    brakMiejsca = true;
    poczatekOstrzezenia = GetTickCount64();
    return;
}

void DodajOkrag() {
    if (czyMiejsceWolneOkrag() && czyMiejsceWolneKwadrat() && czyMiejsceWolneTrojkat()) {
        Okrag NowyOkrag;
        NowyOkrag.x = POCZATEK_SZYNY - 20;
        NowyOkrag.y = POZIOM_PODLOGI;
        NowyOkrag.czyMagnes = false;
        NowyOkrag.spadanie = false;
        NowyOkrag.masa = masaDodawanegoKlocka;
        okregi.push_back(NowyOkrag);
        brakMiejsca = false;
        return;
    }
    brakMiejsca = true;
    poczatekOstrzezenia = GetTickCount64();
    return;
}

void DodajTrojkat() {
    if (czyMiejsceWolneTrojkat() && czyMiejsceWolneKwadrat() && czyMiejsceWolneOkrag()) {
        Trojkat nowyTrojkat;
        nowyTrojkat.x = POCZATEK_SZYNY - 20;
        nowyTrojkat.y = POZIOM_PODLOGI;
        nowyTrojkat.czyMagnes = false;
        nowyTrojkat.spadanie = false;
        nowyTrojkat.masa = masaDodawanegoKlocka;
        trojkaty.push_back(nowyTrojkat);
        brakMiejsca = false;
        return;
    }
    brakMiejsca = true;
    poczatekOstrzezenia = GetTickCount64();
    return;
}


void zmienTryb(TypTrybu nowyTryb) {
    if (czyMagnesDziala) {
        czyMagnesDziala = false;
    }
    usuwanieKlockow();
    aktualnyTryb = nowyTryb;
}

void przelaczMagnes() {
    if (!czyMagnesDziala) {
        czyMagnesDziala = true;


        if (aktualnyTryb == TRYB_KWADRATY) {
            int indeksKlocka = sprawdzenieKlockaPodMagnesem();
            if (indeksKlocka != -1) {
                if (kwadraty[indeksKlocka].masa < 7) {
                    indeksPrzyczepionegoKwadratu = indeksKlocka;
                    kwadraty[indeksPrzyczepionegoKwadratu].czyMagnes = true;
                    kwadraty[indeksPrzyczepionegoKwadratu].spadanie = false;
                }
                else {
                    bladMasa = true;
                    poczatekOstrzezeniaMasa = GetTickCount64();
                }
            }
        }
        else if (aktualnyTryb == TRYB_OKREGI) {
            int indeksOkragu = sprawdzenieOkraguPodMagnesem();
            if (indeksOkragu != -1) {
                if (okregi[indeksOkragu].masa < 7) {
                    indeksPrzyczepionegoOkragu = indeksOkragu;
                    okregi[indeksOkragu].czyMagnes = true;
                    okregi[indeksOkragu].spadanie = false;
                }
                else {
                    bladMasa = true;
                    poczatekOstrzezeniaMasa = GetTickCount64();
                }
            }
        }
        else if (aktualnyTryb == TRYB_TROJKATY) {
            int indeksTrojkata = sprawdzenieTrojkataPodMagnesem();
            if (indeksTrojkata != -1) {
                if (trojkaty[indeksTrojkata].masa < 7) {
                    indeksPrzyczepionegoTrojkata = indeksTrojkata;
                    trojkaty[indeksTrojkata].czyMagnes = true;
                    trojkaty[indeksTrojkata].spadanie = false;
                }
                else {
                    bladMasa = true;
                    poczatekOstrzezeniaMasa = GetTickCount64();
                }
            }
        }
    }
    else {

        if (czyMoznaWylaczycMagnes()) {
            czyMagnesDziala = false;

            if (indeksPrzyczepionegoKwadratu != -1) {
                kwadraty[indeksPrzyczepionegoKwadratu].spadanie = true;
                kwadraty[indeksPrzyczepionegoKwadratu].czyMagnes = false;
                indeksPrzyczepionegoKwadratu = -1;
            }
            if (indeksPrzyczepionegoOkragu != -1) {
                okregi[indeksPrzyczepionegoOkragu].spadanie = true;
                okregi[indeksPrzyczepionegoOkragu].czyMagnes = false;
                indeksPrzyczepionegoOkragu = -1;
            }
            if (indeksPrzyczepionegoTrojkata != -1) {
                trojkaty[indeksPrzyczepionegoTrojkata].spadanie = true;
                trojkaty[indeksPrzyczepionegoTrojkata].czyMagnes = false;
                indeksPrzyczepionegoTrojkata = -1;
            }
        }
        else {
            if (nowyDzwigY <= 430) {
                bladMagnes = true;
                poczatekOstrzezeniaBHP = GetTickCount64();
            }
            else {
                bladTryb = true;
                poczatekOstrzezeniaTryb = GetTickCount64();
            }
            return;
        }
    }
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
    {
        GdiplusShutdown(gdiplusToken);
        return FALSE;
    }

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    GdiplusShutdown(gdiplusToken);
    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    DWORD windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT windowRect = { 0, 0, 1280, 720 };
    AdjustWindowRect(&windowRect, windowStyle, FALSE);

    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, windowStyle,
        CW_USEDEFAULT, 0, windowWidth, windowHeight, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        SetTimer(hWnd, timerId, 33, NULL);
        break;
    case WM_TIMER: {
        if (brakMiejsca && (GetTickCount64() - poczatekOstrzezenia) > CZAS_OSTRZEZENIA) {
            brakMiejsca = false;
            InvalidateRect(hWnd, NULL, TRUE);
        }

        if (bladMagnes && (GetTickCount64() - poczatekOstrzezeniaBHP) > CZAS_OSTRZEZENIA) {
            bladMagnes = false;
            InvalidateRect(hWnd, NULL, TRUE);
        }

        if (bladTryb && (GetTickCount64() - poczatekOstrzezeniaTryb) > CZAS_OSTRZEZENIA) {
            bladTryb = false;
            InvalidateRect(hWnd, NULL, TRUE);
        }
        
        if (bladMasa && (GetTickCount64() - poczatekOstrzezeniaMasa) > CZAS_OSTRZEZENIA) {
            bladMasa = false;
            InvalidateRect(hWnd, NULL, TRUE);
        }

        bool czyAktualizowacEkran = false;

        for (int i = 0; i < kwadraty.size(); i++) {
            if (i == indeksPrzyczepionegoKwadratu) continue;

            auto& kwadrat = kwadraty[i];

            if (!kwadrat.spadanie) {
                bool maPodparcie = false;

                if (kwadrat.y >= POZIOM_PODLOGI) maPodparcie = true;
                else {
                    if (aktualnyTryb == TRYB_KWADRATY) {
                        for (int j = 0; j < kwadraty.size(); j++) {
                            if (j == i) continue;
                            const auto& drugiKwadrat = kwadraty[j];
                            if (kwadrat.y + ROZMIAR_KLOCKA == drugiKwadrat.y &&
                                kwadrat.x < drugiKwadrat.x + ROZMIAR_KLOCKA &&
                                kwadrat.x + ROZMIAR_KLOCKA > drugiKwadrat.x) {
                                maPodparcie = true;
                                break;
                            }
                        }
                    }


                    if (!maPodparcie && aktualnyTryb != TRYB_KWADRATY) {
                        for (const auto& trojkat : trojkaty) {
                            if (kwadrat.y + ROZMIAR_KLOCKA == trojkat.y &&
                                kwadrat.x < trojkat.x + ROZMIAR_TROJKATA &&
                                kwadrat.x + ROZMIAR_KLOCKA > trojkat.x) {
                                maPodparcie = true;
                                break;
                            }
                        }
                    }


                    if (!maPodparcie && aktualnyTryb != TRYB_KWADRATY) {
                        for (const auto& okrag : okregi) {
                            if (kwadrat.y + ROZMIAR_KLOCKA == okrag.y &&
                                kwadrat.x < okrag.x + ROZMIAR_OKRAGU_X &&
                                kwadrat.x + ROZMIAR_KLOCKA > okrag.x) {
                                maPodparcie = true;
                                break;
                            }
                        }
                    }
                }

                if (!maPodparcie && czyMozliweSpadanie(i)) {
                    kwadrat.spadanie = true;
                }
            }

            if (kwadrat.spadanie) {
                int nowyY = kwadrat.y + KLOCKI_PREDKOSC_Y;
                bool kolizja = false;
                czyAktualizowacEkran = true;


                for (int j = 0; j < kwadraty.size(); j++) {
                    if (j == i) continue;

                    const auto& drugiKwadrat = kwadraty[j];
                    if (sprawdzenieKolizji(kwadrat.x, nowyY, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA,
                        drugiKwadrat.x, drugiKwadrat.y, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA)) {
                        kwadrat.y = drugiKwadrat.y - ROZMIAR_KLOCKA;
                        kwadrat.spadanie = false;
                        kolizja = true;
                        break;
                    }
                }


                if (!kolizja) {
                    for (const auto& trojkat : trojkaty) {
                        if (sprawdzenieKolizji(kwadrat.x, nowyY, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA,
                            trojkat.x, trojkat.y, ROZMIAR_TROJKATA, ROZMIAR_TROJKATA)) {
                            kwadrat.y = trojkat.y - ROZMIAR_KLOCKA;
                            kwadrat.spadanie = false;
                            kolizja = true;
                            break;
                        }
                    }
                }


                if (!kolizja) {
                    for (const auto& okrag : okregi) {
                        if (sprawdzenieKolizji(kwadrat.x, nowyY, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA,
                            okrag.x, okrag.y, ROZMIAR_OKRAGU_X, ROZMIAR_OKRAGU_Y)) {
                            kwadrat.y = okrag.y - ROZMIAR_KLOCKA;
                            kwadrat.spadanie = false;
                            kolizja = true;
                            break;
                        }
                    }
                }

                if (!kolizja) {
                    kwadrat.y = nowyY;
                    if (kwadrat.y >= POZIOM_PODLOGI) {
                        kwadrat.y = POZIOM_PODLOGI;
                        kwadrat.spadanie = false;
                    }
                }
            }
        }

        for (int i = 0; i < trojkaty.size(); i++) {
            if (i == indeksPrzyczepionegoTrojkata) continue;

            auto& trojkat = trojkaty[i];
            if (!trojkat.spadanie) {
                bool maPodparcie = false;

                if (trojkat.y >= POZIOM_PODLOGI) maPodparcie = true;
                else {
                    if (aktualnyTryb == TRYB_TROJKATY) {
                        for (int j = 0; j < trojkaty.size(); j++) {
                            if (j == i) continue;
                            const auto& drugiTrojkat = trojkaty[j];
                            if (trojkat.y + ROZMIAR_TROJKATA == drugiTrojkat.y &&
                                trojkat.x < drugiTrojkat.x + ROZMIAR_TROJKATA &&
                                trojkat.x + ROZMIAR_TROJKATA > drugiTrojkat.x) {
                                maPodparcie = true;
                                break;
                            }
                        }
                    }
                    if (!maPodparcie && aktualnyTryb != TRYB_TROJKATY) {
                        for (const auto& kwadrat : kwadraty) {
                            if (trojkat.y + ROZMIAR_TROJKATA == kwadrat.y &&
                                trojkat.x < kwadrat.x + ROZMIAR_KLOCKA &&
                                trojkat.x + ROZMIAR_TROJKATA > kwadrat.x) {
                                maPodparcie = true;
                                break;
                            }
                        }
                    }
                    if (!maPodparcie && aktualnyTryb != TRYB_TROJKATY) {
                        for (const auto& okrag : okregi) {
                            if (trojkat.y + ROZMIAR_TROJKATA == okrag.y &&
                                trojkat.x < okrag.x + ROZMIAR_OKRAGU_X &&
                                trojkat.x + ROZMIAR_TROJKATA > okrag.x) {
                                maPodparcie = true;
                                break;
                            }
                        }
                    }
                }

                if (!maPodparcie && czyMozliweSpadanie(i + (int)kwadraty.size() + (int)okregi.size())) {
                    trojkat.spadanie = true;
                }
            }

            if (trojkat.spadanie) {
                int nowyY = trojkat.y + KLOCKI_PREDKOSC_Y;
                bool kolizja = false;
                czyAktualizowacEkran = true;

                for (int j = 0; j < trojkaty.size(); j++) {
                    if (j == i) continue;
                    const auto& drugiTrojkat = trojkaty[j];
                    if (sprawdzenieKolizji(trojkat.x, nowyY, ROZMIAR_TROJKATA, ROZMIAR_TROJKATA,
                        drugiTrojkat.x, drugiTrojkat.y, ROZMIAR_TROJKATA, ROZMIAR_TROJKATA)) {
                        trojkat.y = drugiTrojkat.y - ROZMIAR_TROJKATA;
                        trojkat.spadanie = false;
                        kolizja = true;
                        break;
                    }
                }

                if (!kolizja) {
                    for (const auto& kwadrat : kwadraty) {
                        if (sprawdzenieKolizji(trojkat.x, nowyY, ROZMIAR_TROJKATA, ROZMIAR_TROJKATA,
                            kwadrat.x, kwadrat.y, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA)) {
                            trojkat.y = kwadrat.y - ROZMIAR_TROJKATA;
                            trojkat.spadanie = false;
                            kolizja = true;
                            break;
                        }
                    }
                }

                if (!kolizja) {
                    for (const auto& okrag : okregi) {
                        if (sprawdzenieKolizji(trojkat.x, nowyY, ROZMIAR_TROJKATA, ROZMIAR_TROJKATA,
                            okrag.x, okrag.y, ROZMIAR_OKRAGU_X, ROZMIAR_OKRAGU_Y)) {
                            trojkat.y = okrag.y - ROZMIAR_TROJKATA;
                            trojkat.spadanie = false;
                            kolizja = true;
                            break;
                        }
                    }
                }

                if (!kolizja) {
                    trojkat.y = nowyY;
                    if (trojkat.y >= POZIOM_PODLOGI) {
                        trojkat.y = POZIOM_PODLOGI;
                        trojkat.spadanie = false;
                    }
                }
            }
        }

        for (int i = 0; i < okregi.size(); i++) {
            if (i == indeksPrzyczepionegoOkragu) continue;

            auto& okrag = okregi[i];

            if (!okrag.spadanie) {
                bool maPodparcie = false;

                if (okrag.y >= POZIOM_PODLOGI) maPodparcie = true;
                else {
                    if (aktualnyTryb == TRYB_OKREGI) {
                        for (int j = 0; j < okregi.size(); j++) {
                            if (j == i) continue;
                            const auto& drugiOkrag = okregi[j];
                            if (okrag.y + ROZMIAR_OKRAGU_Y == drugiOkrag.y &&
                                okrag.x < drugiOkrag.x + ROZMIAR_OKRAGU_X &&
                                okrag.x + ROZMIAR_OKRAGU_X > drugiOkrag.x) {
                                maPodparcie = true;
                                break;
                            }
                        }
                    }

                    if (!maPodparcie && aktualnyTryb != TRYB_OKREGI) {
                        for (const auto& kwadrat : kwadraty) {
                            if (okrag.y + ROZMIAR_OKRAGU_Y == kwadrat.y &&
                                okrag.x < kwadrat.x + ROZMIAR_KLOCKA &&
                                okrag.x + ROZMIAR_OKRAGU_X > kwadrat.x) {
                                maPodparcie = true;
                                break;
                            }
                        }
                    }

                    if (!maPodparcie && aktualnyTryb != TRYB_OKREGI) {
                        for (const auto& trojkat : trojkaty) {
                            if (okrag.y + ROZMIAR_OKRAGU_Y == trojkat.y &&
                                okrag.x < trojkat.x + ROZMIAR_TROJKATA &&
                                okrag.x + ROZMIAR_OKRAGU_X > trojkat.x) {
                                maPodparcie = true;
                                break;
                            }
                        }
                    }
                }

                if (!maPodparcie && czyMozliweSpadanie(i + (int)kwadraty.size())) {
                    okrag.spadanie = true;
                }
            }

            if (okrag.spadanie) {
                int nowyY = okrag.y + KLOCKI_PREDKOSC_Y;
                bool kolizja = false;
                czyAktualizowacEkran = true;

                for (int j = 0; j < okregi.size(); j++) {
                    if (j == i) continue;

                    const auto& drugiOkrag = okregi[j];
                    if (sprawdzenieKolizji(okrag.x, nowyY, ROZMIAR_OKRAGU_X, ROZMIAR_OKRAGU_Y,
                        drugiOkrag.x, drugiOkrag.y, ROZMIAR_OKRAGU_X, ROZMIAR_OKRAGU_Y)) {
                        okrag.y = drugiOkrag.y - ROZMIAR_OKRAGU_Y;
                        okrag.spadanie = false;
                        kolizja = true;
                        break;
                    }
                }

                if (!kolizja) {
                    for (const auto& kwadrat : kwadraty) {
                        if (sprawdzenieKolizji(okrag.x, nowyY, ROZMIAR_OKRAGU_X, ROZMIAR_OKRAGU_Y,
                            kwadrat.x, kwadrat.y, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA)) {
                            okrag.y = kwadrat.y - ROZMIAR_OKRAGU_Y;
                            okrag.spadanie = false;
                            kolizja = true;
                            break;
                        }
                    }
                }

                if (!kolizja) {
                    for (const auto& trojkat : trojkaty) {
                        if (sprawdzenieKolizji(okrag.x, nowyY, ROZMIAR_OKRAGU_X, ROZMIAR_OKRAGU_Y,
                            trojkat.x, trojkat.y, ROZMIAR_TROJKATA, ROZMIAR_TROJKATA)) {
                            okrag.y = trojkat.y - ROZMIAR_OKRAGU_Y;
                            okrag.spadanie = false;
                            kolizja = true;
                            break;
                        }
                    }
                }

                if (!kolizja) {
                    okrag.y = nowyY;
                    if (okrag.y >= POZIOM_PODLOGI) {
                        okrag.y = POZIOM_PODLOGI;
                        okrag.spadanie = false;
                    }
                }
            }
        }

        nowyDzwigX = dzwigPozycjaX;
        nowyDzwigY = dzwigPozycjaY;

        if (ruchLewo == true) {
            nowyDzwigX = dzwigPozycjaX - DZWIG_PREDKOSC_X;
            if (nowyDzwigX < 240) nowyDzwigX = 240;
        }
        if (ruchPrawo == true) {
            nowyDzwigX = dzwigPozycjaX + DZWIG_PREDKOSC_X;
            if (nowyDzwigX > 1020) nowyDzwigX = 1020;
        }
        if (ruchGora == true) {
            nowyDzwigY = dzwigPozycjaY - DZWIG_PREDKOSC_Y;
            if (nowyDzwigY < 310) nowyDzwigY = 310;
        }
        if (ruchDol == true) {
            nowyDzwigY = dzwigPozycjaY + DZWIG_PREDKOSC_Y;
            if (nowyDzwigY > 585) nowyDzwigY = 585;
        }

        if ((nowyDzwigX != dzwigPozycjaX || nowyDzwigY != dzwigPozycjaY) && czyMozliwyRuchMagnes(nowyDzwigX, nowyDzwigY)) {
            dzwigPozycjaX = nowyDzwigX;
            dzwigPozycjaY = nowyDzwigY;
            czyAktualizowacEkran = true;

            if (czyMagnesDziala) {
                if (indeksPrzyczepionegoKwadratu != -1) {
                    kwadraty[indeksPrzyczepionegoKwadratu].x = dzwigPozycjaX - 20;
                    kwadraty[indeksPrzyczepionegoKwadratu].y = dzwigPozycjaY + 15;
                }
                if (indeksPrzyczepionegoOkragu != -1) {
                    okregi[indeksPrzyczepionegoOkragu].x = dzwigPozycjaX - 20;
                    okregi[indeksPrzyczepionegoOkragu].y = dzwigPozycjaY + 15;
                }
                if (indeksPrzyczepionegoTrojkata != -1) {
                    trojkaty[indeksPrzyczepionegoTrojkata].x = dzwigPozycjaX - 20;
                    trojkaty[indeksPrzyczepionegoTrojkata].y = dzwigPozycjaY + 15;
                }
            }
        }

        if (czyMagnesDziala)
        {
            if (aktualnyTryb == TRYB_KWADRATY && indeksPrzyczepionegoKwadratu == -1) {
                int nowyKlocek = sprawdzenieKlockaPodMagnesem();
                if (nowyKlocek != -1 && kwadraty[nowyKlocek].masa < 7) {
                    indeksPrzyczepionegoKwadratu = nowyKlocek;
                    kwadraty[indeksPrzyczepionegoKwadratu].czyMagnes = true;
                    kwadraty[indeksPrzyczepionegoKwadratu].spadanie = false;
                    czyAktualizowacEkran = true;
                }
            }
            else if (aktualnyTryb == TRYB_OKREGI && indeksPrzyczepionegoOkragu == -1) {
                int nowyOkrag = sprawdzenieOkraguPodMagnesem();
                if (nowyOkrag != -1 && okregi[nowyOkrag].masa < 7) {
                    indeksPrzyczepionegoOkragu = nowyOkrag;
                    okregi[indeksPrzyczepionegoOkragu].czyMagnes = true;
                    okregi[indeksPrzyczepionegoOkragu].spadanie = false;
                    czyAktualizowacEkran = true;
                }
            }
            else if (aktualnyTryb == TRYB_TROJKATY && indeksPrzyczepionegoTrojkata == -1) {
                int nowyTrojkat = sprawdzenieTrojkataPodMagnesem();
                if (nowyTrojkat != -1 && trojkaty[nowyTrojkat].masa < 7) {
                    indeksPrzyczepionegoTrojkata = nowyTrojkat;
                    trojkaty[indeksPrzyczepionegoTrojkata].czyMagnes = true;
                    trojkaty[indeksPrzyczepionegoTrojkata].spadanie = false;
                    czyAktualizowacEkran = true;
                }
            }
        }
        if (czyAktualizowacEkran) InvalidateRect(hWnd, NULL, TRUE);
    }
                 break;
    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* pMinMax = (MINMAXINFO*)lParam;
        RECT rect = { 0, 0, 1280, 720 };
        AdjustWindowRect(&rect, GetWindowLong(hWnd, GWL_STYLE), FALSE);

        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        pMinMax->ptMinTrackSize.x = width;
        pMinMax->ptMinTrackSize.y = height;
        pMinMax->ptMaxTrackSize.x = width;
        pMinMax->ptMaxTrackSize.y = height;
    }
    break;
    case WM_LBUTTONDOWN: {
        int kursorX = LOWORD(lParam);
        int kursorY = HIWORD(lParam);

        if (kursorX >= przyciskDodajKwadrat.x && kursorX <= przyciskDodajKwadrat.x + przyciskDodajKwadrat.szer &&
            kursorY >= przyciskDodajKwadrat.y && kursorY <= przyciskDodajKwadrat.y + przyciskDodajKwadrat.wys) {
            przyciskDodajKwadrat.czyWcisniety = true;
            break;
        }
        if (kursorX >= przyciskDodajOkrag.x && kursorX <= przyciskDodajOkrag.x + przyciskDodajOkrag.szer &&
            kursorY >= przyciskDodajOkrag.y && kursorY <= przyciskDodajOkrag.y + przyciskDodajOkrag.wys) {
            przyciskDodajOkrag.czyWcisniety = true;
            break;
        }
        if (kursorX >= przyciskDodajTrojkat.x && kursorX <= przyciskDodajTrojkat.x + przyciskDodajTrojkat.szer &&
            kursorY >= przyciskDodajTrojkat.y && kursorY <= przyciskDodajTrojkat.y + przyciskDodajTrojkat.wys) {
            przyciskDodajTrojkat.czyWcisniety = true;
            break;
        }
        if (kursorX >= przyciskWlaczMagnes.x && kursorX <= przyciskWlaczMagnes.x + przyciskWlaczMagnes.szer &&
            kursorY >= przyciskWlaczMagnes.y && kursorY <= przyciskWlaczMagnes.y + przyciskWlaczMagnes.wys) {
            przyciskWlaczMagnes.czyWcisniety = true;
            break;
        }
        if (kursorX >= przyciskUsunKlocki.x && kursorX <= przyciskUsunKlocki.x + przyciskUsunKlocki.szer &&
            kursorY >= przyciskUsunKlocki.y && kursorY <= przyciskUsunKlocki.y + przyciskUsunKlocki.wys) {
            przyciskUsunKlocki.czyWcisniety = true;
            break;
        }
        if (kursorX >= przyciskZmienTrybKwadraty.x && kursorX <= przyciskZmienTrybKwadraty.x + przyciskZmienTrybKwadraty.szer &&
            kursorY >= przyciskZmienTrybKwadraty.y && kursorY <= przyciskZmienTrybKwadraty.y + przyciskZmienTrybKwadraty.wys) {
            przyciskZmienTrybKwadraty.czyWcisniety = true;
            break;
        }
        if (kursorX >= przyciskZmienTrybOkregi.x && kursorX <= przyciskZmienTrybOkregi.x + przyciskZmienTrybOkregi.szer &&
            kursorY >= przyciskZmienTrybOkregi.y && kursorY <= przyciskZmienTrybOkregi.y + przyciskZmienTrybOkregi.wys) {
            przyciskZmienTrybOkregi.czyWcisniety = true;
            break;
        }
        if (kursorX >= przyciskZmienTrybTrojkaty.x && kursorX <= przyciskZmienTrybTrojkaty.x + przyciskZmienTrybTrojkaty.szer &&
            kursorY >= przyciskZmienTrybTrojkaty.y && kursorY <= przyciskZmienTrybTrojkaty.y + przyciskZmienTrybTrojkaty.wys) {
            przyciskZmienTrybTrojkaty.czyWcisniety = true;
            break;
        }
        if (kursorX >= przyciskMinus.x && kursorX <= przyciskMinus.x + przyciskMinus.szer &&
            kursorY >= przyciskMinus.y && kursorY <= przyciskMinus.y + przyciskMinus.wys) {
            przyciskMinus.czyWcisniety = true;
            break;
        }
        if (kursorX >= przyciskPlus.x && kursorX <= przyciskPlus.x + przyciskPlus.szer &&
            kursorY >= przyciskPlus.y && kursorY <= przyciskPlus.y + przyciskPlus.wys) {
            przyciskPlus.czyWcisniety = true;
            break;
        }

    }
                       break;
    case WM_LBUTTONUP: {
        if (przyciskDodajKwadrat.czyWcisniety == true) {
            przyciskDodajKwadrat.czyWcisniety = false;
            DodajKwadrat();
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        }
        if (przyciskDodajOkrag.czyWcisniety == true) {
            przyciskDodajOkrag.czyWcisniety = false;
            DodajOkrag();
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        }
        if (przyciskDodajTrojkat.czyWcisniety == true) {
            przyciskDodajTrojkat.czyWcisniety = false;
            DodajTrojkat();
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        }
        if (przyciskWlaczMagnes.czyWcisniety == true) {
            przyciskWlaczMagnes.czyWcisniety = false;
            przelaczMagnes();
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        }
        if (przyciskUsunKlocki.czyWcisniety == true) {
            przyciskUsunKlocki.czyWcisniety = false;
            usuwanieKlockow();
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        }
        if (przyciskZmienTrybKwadraty.czyWcisniety == true) {
            przyciskZmienTrybKwadraty.czyWcisniety = false;
            zmienTryb(TRYB_KWADRATY);
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        }
        if (przyciskZmienTrybOkregi.czyWcisniety == true) {
            przyciskZmienTrybOkregi.czyWcisniety = false;
            zmienTryb(TRYB_OKREGI);
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        }

        if (przyciskZmienTrybTrojkaty.czyWcisniety == true) {
            przyciskZmienTrybTrojkaty.czyWcisniety = false;
            zmienTryb(TRYB_TROJKATY);
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        }
        if (przyciskMinus.czyWcisniety == true) {
            przyciskMinus.czyWcisniety = false;
            masaDodawanegoKlocka--;
            if (masaDodawanegoKlocka < 1) masaDodawanegoKlocka = 1;
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        }
        if (przyciskPlus.czyWcisniety == true) {
            przyciskPlus.czyWcisniety = false;
            masaDodawanegoKlocka++;
            if (masaDodawanegoKlocka > 10) masaDodawanegoKlocka = 10;
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        }
    }
                     break;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            DestroyWindow(hWnd);
        }
        else if (wParam == VK_LEFT) {
            ruchLewo = true;
        }
        else if (wParam == VK_RIGHT) {
            ruchPrawo = true;
        }
        else if (wParam == VK_UP) {
            ruchGora = true;
        }
        else if (wParam == VK_DOWN) {
            ruchDol = true;
        }
        break;

    case WM_KEYUP:
        if (wParam == VK_LEFT) {
            ruchLewo = false;
        }
        else if (wParam == VK_RIGHT) {
            ruchPrawo = false;
        }
        else if (wParam == VK_UP) {
            ruchGora = false;
        }
        else if (wParam == VK_DOWN) {
            ruchDol = false;
        }
        break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        Graphics graphics(hdc);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        FontFamily rodzinaCzcionek(L"Times New Roman");
        StringFormat format;
        format.SetAlignment(StringAlignmentCenter);
        format.SetLineAlignment(StringAlignmentCenter);
        //sta³e, wymyœl coœ, ¿eby pozbyæ siê flickeringu!!
        Pen obramowaniePen(Color(255, 0, 0, 0), 3);
        Rect obramowanie(140, 60, 1000, 600);
        graphics.DrawRectangle(&obramowaniePen, obramowanie);

        Pen nogaDzwiguPen(Color(255, 255, 251, 0), 1);
        SolidBrush nogaDzwiguBrush(Color(255, 255, 251, 0));
        Rect nogaDzwigu(600, 210, 80, 450);
        graphics.DrawRectangle(&nogaDzwiguPen, nogaDzwigu);
        graphics.FillRectangle(&nogaDzwiguBrush, nogaDzwigu);

        Rect szynaDzwigu(240, 140, 800, 70);
        graphics.DrawRectangle(&nogaDzwiguPen, szynaDzwigu);
        graphics.FillRectangle(&nogaDzwiguBrush, szynaDzwigu);

        //zmieniaj¹ce siê, raczej bêd¹ flickerowaæ
        SolidBrush klocekBrush(Color(255, 158, 144, 122));
        Pen klocekPen(Color(255, 61, 51, 37), 2);
        Font czcionkaKlocek(&rodzinaCzcionek, 16, FontStyleBold, UnitPixel);
        SolidBrush klocekTekstBrush(Color(255, 255, 255, 255));

        for (auto const& kwadrat : kwadraty) {
            Rect kwadratRect(kwadrat.x, kwadrat.y, 60, 60);
            graphics.DrawRectangle(&klocekPen, kwadratRect);
            graphics.FillRectangle(&klocekBrush, kwadratRect);
            wchar_t klocekTekst[3];
            swprintf_s(klocekTekst, L"%d", kwadrat.masa);
            RectF klocekTekstRect(kwadrat.x, kwadrat.y, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA);

            graphics.DrawString(klocekTekst, -1, &czcionkaKlocek, klocekTekstRect, &format, &klocekTekstBrush);
        }
        for (auto const& Okrag : okregi) {
            Rect circleRect(Okrag.x, Okrag.y, 60, 60);
            graphics.DrawEllipse(&klocekPen, circleRect);
            graphics.FillEllipse(&klocekBrush, circleRect);
            wchar_t klocekTekst[3];
            swprintf_s(klocekTekst, L"%d", Okrag.masa);
            RectF klocekTekstRect(Okrag.x, Okrag.y, ROZMIAR_KLOCKA, ROZMIAR_KLOCKA);

            graphics.DrawString(klocekTekst, -1, &czcionkaKlocek, klocekTekstRect, &format, &klocekTekstBrush);
        }
        for (auto const& trojkat : trojkaty) {
            float x = trojkat.x;
            float y = trojkat.y;
            PointF points[3] = {
                PointF(x + 30, y + 0),
                PointF(x + 0, y + 60),
                PointF(x + 60, y + 60)
            };
            graphics.FillPolygon(&klocekBrush, points, 3);
            graphics.DrawPolygon(&klocekPen, points, 3);
            wchar_t klocekTekst[3];
            swprintf_s(klocekTekst, L"%d", trojkat.masa);
            RectF klocekTekstRect(trojkat.x, trojkat.y+20, ROZMIAR_KLOCKA, 40);

            graphics.DrawString(klocekTekst, -1, &czcionkaKlocek, klocekTekstRect, &format, &klocekTekstBrush);
        }
        SolidBrush uchwytDzwiguBrush(Color(255, 158, 158, 158));
        Pen uchwytDzwiguPen(Color(255, 82, 82, 82), 2);
        Rect uchwytDzwigu(dzwigPozycjaX, 140, 20, 70);
        graphics.DrawRectangle(&uchwytDzwiguPen, uchwytDzwigu);
        graphics.FillRectangle(&uchwytDzwiguBrush, uchwytDzwigu);

        Point mocowanieLinaUchwyt(dzwigPozycjaX + 10, 210);
        Point mocowanieLinaMagnes(dzwigPozycjaX + 10, dzwigPozycjaY);

        Pen linaPen(Color(255, 0, 0, 0), 4);
        graphics.DrawLine(&linaPen, mocowanieLinaUchwyt, mocowanieLinaMagnes);

        SolidBrush magnesWlaczonyBrush(Color(255, 0, 0, 200));

        Rect magnesDzwigu(dzwigPozycjaX - 15, dzwigPozycjaY, 50, 15);
        graphics.DrawRectangle(&uchwytDzwiguPen, magnesDzwigu);
        if (!czyMagnesDziala) graphics.FillRectangle(&uchwytDzwiguBrush, magnesDzwigu);
        else graphics.FillRectangle(&magnesWlaczonyBrush, magnesDzwigu);

        //przyciski
        Pen przyciskPen(Color(255, 100, 100, 100), 2);
        SolidBrush przyciskBrush(Color(255, 200, 200, 200));
        Font przyciskCzcionka(&rodzinaCzcionek, 24, FontStyleRegular, UnitPixel);
        SolidBrush przyciskCzcionkaBrush(Color(255, 0, 0, 0));

        RectF przyciskDodajKwadratTekstRect(przyciskDodajKwadrat.x, przyciskDodajKwadrat.y, przyciskDodajKwadrat.szer, przyciskDodajKwadrat.wys);
        Rect przyciskDodajKwadratRect(przyciskDodajKwadrat.x, przyciskDodajKwadrat.y, przyciskDodajKwadrat.szer, przyciskDodajKwadrat.wys);
        graphics.DrawRectangle(&przyciskPen, przyciskDodajKwadratRect);
        graphics.FillRectangle(&przyciskBrush, przyciskDodajKwadratRect);
        graphics.DrawString(przyciskDodajKwadrat.napis, -1, &przyciskCzcionka, przyciskDodajKwadratTekstRect, &format, &przyciskCzcionkaBrush);

        RectF przyciskDodajOkragTekstRect(przyciskDodajOkrag.x, przyciskDodajOkrag.y, przyciskDodajOkrag.szer, przyciskDodajOkrag.wys);
        Rect przyciskDodajOkragRect(przyciskDodajOkrag.x, przyciskDodajOkrag.y, przyciskDodajOkrag.szer, przyciskDodajOkrag.wys);
        graphics.DrawRectangle(&przyciskPen, przyciskDodajOkragRect);
        graphics.FillRectangle(&przyciskBrush, przyciskDodajOkragRect);
        graphics.DrawString(przyciskDodajOkrag.napis, -1, &przyciskCzcionka, przyciskDodajOkragTekstRect, &format, &przyciskCzcionkaBrush);

        RectF przyciskDodajTrojkatTekstRect(przyciskDodajTrojkat.x, przyciskDodajTrojkat.y, przyciskDodajTrojkat.szer, przyciskDodajTrojkat.wys);
        Rect przyciskDodajTrojkatRect(przyciskDodajTrojkat.x, przyciskDodajTrojkat.y, przyciskDodajTrojkat.szer, przyciskDodajTrojkat.wys);
        graphics.DrawRectangle(&przyciskPen, przyciskDodajTrojkatRect);
        graphics.FillRectangle(&przyciskBrush, przyciskDodajTrojkatRect);
        graphics.DrawString(przyciskDodajTrojkat.napis, -1, &przyciskCzcionka, przyciskDodajTrojkatTekstRect, &format, &przyciskCzcionkaBrush);

        RectF przyciskUsunKlockiTekstRect(przyciskUsunKlocki.x, przyciskUsunKlocki.y, przyciskUsunKlocki.szer, przyciskUsunKlocki.wys);
        Rect przyciskUsunKlockiRect(przyciskUsunKlocki.x, przyciskUsunKlocki.y, przyciskUsunKlocki.szer, przyciskUsunKlocki.wys);
        graphics.DrawRectangle(&przyciskPen, przyciskUsunKlockiRect);
        graphics.FillRectangle(&przyciskBrush, przyciskUsunKlockiRect);
        graphics.DrawString(przyciskUsunKlocki.napis, -1, &przyciskCzcionka, przyciskUsunKlockiTekstRect, &format, &przyciskCzcionkaBrush);

        RectF przyciskZmienTrybKwadratyTekstRect(przyciskZmienTrybKwadraty.x, przyciskZmienTrybKwadraty.y, przyciskZmienTrybKwadraty.szer, przyciskZmienTrybKwadraty.wys);
        Rect przyciskZmienTrybKwadratyRect(przyciskZmienTrybKwadraty.x, przyciskZmienTrybKwadraty.y, przyciskZmienTrybKwadraty.szer, przyciskZmienTrybKwadraty.wys);
        graphics.DrawRectangle(&przyciskPen, przyciskZmienTrybKwadratyRect);
        graphics.FillRectangle(&przyciskBrush, przyciskZmienTrybKwadratyRect);
        graphics.DrawString(przyciskZmienTrybKwadraty.napis, -1, &przyciskCzcionka, przyciskZmienTrybKwadratyTekstRect, &format, &przyciskCzcionkaBrush);

        RectF przyciskZmienTrybOkregiTekstRect(przyciskZmienTrybOkregi.x, przyciskZmienTrybOkregi.y, przyciskZmienTrybOkregi.szer, przyciskZmienTrybOkregi.wys);
        Rect przyciskZmienTrybOkregiRect(przyciskZmienTrybOkregi.x, przyciskZmienTrybOkregi.y, przyciskZmienTrybOkregi.szer, przyciskZmienTrybOkregi.wys);
        graphics.DrawRectangle(&przyciskPen, przyciskZmienTrybOkregiRect);
        graphics.FillRectangle(&przyciskBrush, przyciskZmienTrybOkregiRect);
        graphics.DrawString(przyciskZmienTrybOkregi.napis, -1, &przyciskCzcionka, przyciskZmienTrybOkregiTekstRect, &format, &przyciskCzcionkaBrush);

        RectF przyciskZmienTrybTrojkatyTekstRect(przyciskZmienTrybTrojkaty.x, przyciskZmienTrybTrojkaty.y, przyciskZmienTrybTrojkaty.szer, przyciskZmienTrybTrojkaty.wys);
        Rect przyciskZmienTrybTrojkatyRect(przyciskZmienTrybTrojkaty.x, przyciskZmienTrybTrojkaty.y, przyciskZmienTrybTrojkaty.szer, przyciskZmienTrybTrojkaty.wys);
        graphics.DrawRectangle(&przyciskPen, przyciskZmienTrybTrojkatyRect);
        graphics.FillRectangle(&przyciskBrush, przyciskZmienTrybTrojkatyRect);
        graphics.DrawString(przyciskZmienTrybTrojkaty.napis, -1, &przyciskCzcionka, przyciskZmienTrybTrojkatyTekstRect, &format, &przyciskCzcionkaBrush);

        RectF przyciskMinusTekstRect(przyciskMinus.x, przyciskMinus.y, przyciskMinus.szer, przyciskMinus.wys);
        Rect przyciskMinusRect(przyciskMinus.x, przyciskMinus.y, przyciskMinus.szer, przyciskMinus.wys);
        graphics.DrawRectangle(&przyciskPen, przyciskMinusRect);
        graphics.FillRectangle(&przyciskBrush, przyciskMinusRect);
        graphics.DrawString(przyciskMinus.napis, -1, &przyciskCzcionka, przyciskMinusTekstRect, &format, &przyciskCzcionkaBrush);

        RectF przyciskPlusTekstRect(przyciskPlus.x, przyciskPlus.y, przyciskPlus.szer, przyciskPlus.wys);
        Rect przyciskPlusRect(przyciskPlus.x, przyciskPlus.y, przyciskPlus.szer, przyciskPlus.wys);
        graphics.DrawRectangle(&przyciskPen, przyciskPlusRect);
        graphics.FillRectangle(&przyciskBrush, przyciskPlusRect);
        graphics.DrawString(przyciskPlus.napis, -1, &przyciskCzcionka, przyciskPlusTekstRect, &format, &przyciskCzcionkaBrush);

        wchar_t tekstMasy[40];
        swprintf_s(tekstMasy, L"Masa kolejnego klocka: %d", masaDodawanegoKlocka);
        RectF masaRect(5, 180, 130, 100);
        graphics.DrawString(tekstMasy, -1, &przyciskCzcionka, masaRect, &format, &przyciskCzcionkaBrush);

        wchar_t komunikatTryb[50] = L"Aktualnie wybrany tryb: ";
        if (aktualnyTryb == TRYB_KWADRATY) {
            wcscat_s(komunikatTryb, L"kwadraty.");
        }
        else if (aktualnyTryb == TRYB_OKREGI) {
            wcscat_s(komunikatTryb, L"okregi.");
        }
        else {
            wcscat_s(komunikatTryb, L"trojkaty.");
        }
        RectF komunikatRect(5, 300, 130, 200);
        graphics.DrawString(komunikatTryb, -1, &przyciskCzcionka, komunikatRect, &format, &przyciskCzcionkaBrush);

        const wchar_t* tekstPrzyciskMagnes = czyMagnesDziala ? L"Wylacz magnes" : L"Wlacz magnes";
        RectF przyciskWlaczMagnesTekstRect(przyciskWlaczMagnes.x, przyciskWlaczMagnes.y, przyciskWlaczMagnes.szer, przyciskWlaczMagnes.wys);
        Rect przyciskWlaczMagnesRect(przyciskWlaczMagnes.x, przyciskWlaczMagnes.y, przyciskWlaczMagnes.szer, przyciskWlaczMagnes.wys);
        graphics.DrawRectangle(&przyciskPen, przyciskWlaczMagnesRect);
        graphics.FillRectangle(&przyciskBrush, przyciskWlaczMagnesRect);
        graphics.DrawString(tekstPrzyciskMagnes, -1, &przyciskCzcionka, przyciskWlaczMagnesTekstRect, &format, &przyciskCzcionkaBrush);

        //ostrze¿enia: brak miejsca dla kloca, blad bhp wysokosc, blad bhp zly tryb, blad za duza masa
        if (brakMiejsca) {
            SolidBrush ostrzezenieBrush(Color(200, 255, 0, 0));
            Pen ostrzezeniePen(Color(255, 200, 0, 0), 3);
            Font ostrzezenieFont(&rodzinaCzcionek, 32, FontStyleBold, UnitPixel);
            SolidBrush ostrzezenieTextBrush(Color(255, 255, 255, 255));

            RectF ostrzezenieRect(400, 300, 480, 120);
            graphics.FillRectangle(&ostrzezenieBrush, ostrzezenieRect);
            graphics.DrawRectangle(&ostrzezeniePen, Rect(400, 300, 480, 120));

            StringFormat ostrzezenieFormat;
            ostrzezenieFormat.SetAlignment(StringAlignmentCenter);
            ostrzezenieFormat.SetLineAlignment(StringAlignmentCenter);
            graphics.DrawString(L"BRAK MIEJSCA\nNA KLOCEK!", -1, &ostrzezenieFont, ostrzezenieRect, &ostrzezenieFormat, &ostrzezenieTextBrush);
        }
        if (bladMagnes) {
            SolidBrush ostrzezenieBrush(Color(200, 255, 0, 0));
            Pen ostrzezeniePen(Color(255, 200, 0, 0), 3);
            Font ostrzezenieFont(&rodzinaCzcionek, 32, FontStyleBold, UnitPixel);
            SolidBrush ostrzezenieTextBrush(Color(255, 255, 255, 255));

            RectF ostrzezenieRect(400, 300, 480, 120);
            graphics.FillRectangle(&ostrzezenieBrush, ostrzezenieRect);
            graphics.DrawRectangle(&ostrzezeniePen, Rect(400, 300, 480, 120));

            StringFormat ostrzezenieFormat;
            ostrzezenieFormat.SetAlignment(StringAlignmentCenter);
            ostrzezenieFormat.SetLineAlignment(StringAlignmentCenter);
            graphics.DrawString(L"BLAD BHP, ZA WYSOKO\nZEBY WYLACZYC MAGNES", -1, &ostrzezenieFont, ostrzezenieRect, &ostrzezenieFormat, &ostrzezenieTextBrush);
        }
        if (bladTryb) {
            SolidBrush ostrzezenieBrush(Color(200, 255, 0, 0));
            Pen ostrzezeniePen(Color(255, 200, 0, 0), 3);
            Font ostrzezenieFont(&rodzinaCzcionek, 32, FontStyleBold, UnitPixel);
            SolidBrush ostrzezenieTextBrush(Color(255, 255, 255, 255));

            RectF ostrzezenieRect(400, 430, 480, 120);
            graphics.FillRectangle(&ostrzezenieBrush, ostrzezenieRect);
            graphics.DrawRectangle(&ostrzezeniePen, Rect(400, 430, 480, 120));

            StringFormat ostrzezenieFormat;
            ostrzezenieFormat.SetAlignment(StringAlignmentCenter);
            ostrzezenieFormat.SetLineAlignment(StringAlignmentCenter);
            graphics.DrawString(L"BLAD BHP, NIE MOZNA\nODSTAWIC NA TYM TYPIE", -1, &ostrzezenieFont, ostrzezenieRect, &ostrzezenieFormat, &ostrzezenieTextBrush);
        }
        if (bladMasa) {
            SolidBrush ostrzezenieBrush(Color(200, 255, 0, 0));
            Pen ostrzezeniePen(Color(255, 200, 0, 0), 3);
            Font ostrzezenieFont(&rodzinaCzcionek, 32, FontStyleBold, UnitPixel);
            SolidBrush ostrzezenieTextBrush(Color(255, 255, 255, 255));

            RectF ostrzezenieRect(400, 300, 480, 120);
            graphics.FillRectangle(&ostrzezenieBrush, ostrzezenieRect);
            graphics.DrawRectangle(&ostrzezeniePen, Rect(400, 300, 480, 120));

            StringFormat ostrzezenieFormat;
            ostrzezenieFormat.SetAlignment(StringAlignmentCenter);
            ostrzezenieFormat.SetLineAlignment(StringAlignmentCenter);
            graphics.DrawString(L"BLAD BHP, ZA DUZA\nMASA KLOCKA", -1, &ostrzezenieFont, ostrzezenieRect, &ostrzezenieFormat, &ostrzezenieTextBrush);
        }

        EndPaint(hWnd, &ps);
    }
    break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}