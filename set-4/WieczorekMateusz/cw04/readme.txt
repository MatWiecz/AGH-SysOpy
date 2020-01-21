Zadania 1-3:
Pliki źródłowe należy kompilować za pomocą sekwencji komend: "cmake ." oraz "make".
Z aplikacji należy korzystać w sposób opisany w poleceniach zadań.

Zadanie 3a:
W przypadku użycia trybu "SIGRT" odbierane są wszystkie sygnały, zarówno przez sender
jak i catcher, ponieważ sygnały czasu rzeczywistego są kolejkowane, a więc
żaden z nich nie jest pomijany.

Zadanie 3b:
W przypadku dużej liczby sygnałów do przesłania aplikacje się zakleszczają
nawzajem.
