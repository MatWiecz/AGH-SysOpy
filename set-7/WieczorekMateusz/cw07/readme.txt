Zadania 1-2:
Pliki źródłowe należy kompilować za pomocą sekwencji komend: "cmake ." oraz "make".
Z aplikacji należy korzystać w sposób opisany w poleceniach zadań.

Uruchamianie:
./[sv/posix]-trucker {truck_capacity} {belt_packs_capacity} {belt_weight_capacity}
./[sv/posix]-loader {pack_weigth} {[optional]cycles_number}
./loaders-spawner {loaders_number} {min_pack_weigth} {max_pack_weigth}

Zadanie 2:
Podczas otwierania pamięci współdzielonej przez dużą liczbę loaderów (>20) występuje
problem w dostępie do pamięci wspóldzielonej (program nie loguje żadnych błędów,
ale i nie jest wstanie położyć żadnej paczki na taśmę).
Czasami również nie działa mechanizm oczekiwania na odłączenie się wszystkich
procesów od pamięci wspóldzielonej, przez co nie są usuwane pliki reprezentujące
pamięć współdzieloną.
