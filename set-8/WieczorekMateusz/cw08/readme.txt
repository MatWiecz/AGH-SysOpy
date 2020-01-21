Zadanie 1:
Pliki źródłowe należy kompilować za pomocą sekwencji komend: "cmake ." oraz "make".
Z aplikacji należy korzystać w sposób opisany w poleceniach zadań.
W katalogu zad1 można znaleść skrypt, który po uruchomieniu przeprowadzi testy
i wygeneruje raport do pliku Times.txt.

Uruchamianie:
cmake .
make
./get-times.sh

Analiza wyników:
1. Zwielokrotnienie liczby wątków nie powoduje proporcjonalnego pomniejszenia
czasu wykonywania operacji filtrowanie, tzn. nie opłaca się uruchamiać większej
liczby wątków, gdyż wiąże się to z ich tworzeniem i kończeniem ich pracy.
Dodatkowym aspektem jest otrzymywanie czasu procesora dla poszczególnych wątków,
przez co różnica między tymi czasami jest duża, a program i tak czeka na
zakończenie wszystkich wątków.

Jednak można zauważyć spadek czasu operacji filtrowania.

2. Na podstawie operacji filtrowania przy użyciu filtru my-filter-3 (największy
rozmiar macierzy) można zauważyć, że metoda przetwarzania BLOCK sprawdza się
dla mniejszej liczby wątków (1, 2, 4), natomiast dla większej liczby wątków
leprzym rozwiązaniem bedzie wybór metory INTERLEAVED. Można to uzasadnić w taki
sposób: "Jak wątki mają już skakać co daną liczbę kolumn, to niech skacze ich
dużo".

3. Ilość wątków należy dobierać w taki sposób, aby każdy z nich "porobił trochę
więcej". Nie warto uruchamiać dużej liczby wątków, którę się uruchamiają
i od razu zamykają.
