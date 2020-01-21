Sposób użycia programu main:

create_table [size] - tworzy tabelę o długości size

run_find [dir] [file] [log_file] - uruchamia komendę find, 
	która przeszukuje katalog dir w poszukiwaniu plików file,
	i standardowe wyjście z komendy kieruje do log_file
	
load_file [file] - alokuje pamięć potrzebną do załadowania pliku file
	oraz kopiuje do tego bloku zawartość tego pliku
	
remove_block [index] - zwalnia blok pamięci pod indeksem index
