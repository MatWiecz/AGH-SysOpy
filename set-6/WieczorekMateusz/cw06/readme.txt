Zadania 1-2:
Pliki źródłowe należy kompilować za pomocą sekwencji komend: "cmake ." oraz "make".
Z aplikacji należy korzystać w sposób opisany w poleceniach zadań.

Przykładowe pliki z zestawem komend do wykonania (cmd.txt oraz cmd2.txt
(performance test, rekurencyjne)) można wykonać przy pomocy wpisania
w konsoli klienta "READ cmd(2).txt". Pliki zostały przygotowane dla przypadku
aktywnych pięciu użytkowników, ale oczywiście przy innej liczbie również będą
działać.

W przypadku użycie ADD oraz DEL dla FRIENDS, należy w konsoli klienta wpisać
"FRIENDS ADD/DEL lista_klientów".

Maksymalna liczba podłączonych klientów to 64.

Zadanie 1:
Aby sprawdzić działanie systemu, należy w nowej sesji terminala uruchomić
proces serwera po przez "./my-chat-sv-server", a następnie w nowych sesjach
terminala uruchomić procesy klientóœ po przez "./my-chat-sv-client".

Zadanie 2:
Aby sprawdzić działanie systemu, należy w nowej sesji terminala uruchomić
proces serwera po przez "./my-chat-posix-server", a następnie w nowych sesjach
terminala uruchomić procesy klientóœ po przez "./my-chat-posix-client".
