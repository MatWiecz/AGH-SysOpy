W całości udało mi się wykonać następujące ćwiczenia:
- Zadanie 1: Bibioteki w obu wersjach można otrzymać uruchamiając w katalogu zad1 kolejno: cmake, make.
- Zadanie 2: Analogicznie w ten sam sposób można uzyskać plik wykonywalny main używający
	i testujący funkcje biblioteczne. Po wielu trudnościach udało mi się ostatecznie
	otrzymać w pełni działającą bibliotekę, jak i plik wykonywalny (pliki źródłowe
	dostarczono za pomocą dowiązań symbolicznych)
- Zadanie 3a: Testy zostały przygotowane jedynie dla wersji z biblioteką statyczną i współdzieloną
	jednak nie mogłem na podstawie nich nic wywnioskować, gdyż przy wywołaniu polecenia ctest
	funkcje biblioteki odpowiedzialne za zapis do pliku tymczasowego nie zapisywały żadnej zawartości
	natomiast następywał proces wyszukiwania (dłuższy czas = 1.97 s przy HARD TEST)
- Zadanie 3b. Udało się w Makefile wprowadzić przełączniki ustawiające odpowiednie opcje kompilacji,
	jednak nie udało się sprawdzić ich działania.
