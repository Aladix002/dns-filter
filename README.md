# Filtrujúci DNS resolver

**Autor:** Filip Botlo (xbotlo01)  
**Dátum vytvorenia:** 2025

## Popis programu

Program implementuje filtrujúci DNS resolver v C++17, ktorý blokuje dotazy typu A na domény zo zadaného zoznamu a ich poddomény. Ostatné dotazy prepúšťa v nezmenenej podobe špecifikovanému resolveru. Odpovede na dotazy predáva pôvodnému tazateľovi.

**Rozšírenie:**
- Verbose mód pre ladenie
- Štatistiky na konci behu programu

## Použitie argumentov

```bash
./dns -s server [-p port] -f filter_file [-v] [--stats]
```

### Povinné argumenty:
- **`-s server`** - IP adresa alebo doménové meno DNS serveru (resolveru), kam sa má zaslať dotaz
- **`-f filter_file`** - Meno súboru obsahujúceho nežiaduce domény

### Voliteľné argumenty:
- **`-p port`** - Číslo portu, na ktorom bude program očakávať dotazy (východzia hodnota: 53)
- **`-v`** - Verbose mód - vypisuje informácie o preklade
- **`--stats`** - Na konci behu vypíše štatistiky
- **`-h`** alebo **`--help`** - Zobrazí nápovedu

### Poradie argumentov:
Poradie parametrov je ľubovoľné.

## Príklad spustenia

```bash
# Kompilácia
make

# Základné spustenie s Google DNS
./dns -s 8.8.8.8 -f blocked_domains.txt

# Spustenie s verbose módom na porte 5353
./dns -s 8.8.8.8 -f blocked_domains.txt -p 5353 -v

# Spustenie s IPv6 DNS
./dns -s 2001:4860:4860::8888 -f blocked_domains.txt

# Spustenie so štatistikami
./dns -s 8.8.8.8 -f blocked_domains.txt --stats

# Spustenie s verbose módom a štatistikami
./dns -s 8.8.8.8 -f blocked_domains.txt -p 5353 -v --stats

# Zobrazenie nápovedy
./dns --help

```

## DNS Response kódy

- **REFUSED (5)** - doména je blokovaná
- **NOTIMP (4)** - nepodporovaný typ dotazu (iba A records)
- **NOERROR (0)** - dotaz je prepustený

## Štatistiky

Pri použití argumentu `--stats` program na konci behu vypíše nasledujúce štatistiky:

- **Celkový počet dotazov** - počet všetkých prijatých DNS dotazov
- **Blokované dotazy** - počet dotazov, ktoré boli zablokované (REFUSED)
- **Preposlané dotazy** - počet dotazov, ktoré boli preposlané resolveru
- **Ostatné typy dotazov (NOTIMP)** - počet dotazov s nepodporovanými typmi (AAAA, MX, TXT, NS, CNAME, atď.)
- **Presné zhody s blokovanými doménami** - počet presných zhôd s blokovanými doménami
- **Zhody s poddomenami** - počet zhôd s poddomenami blokovaných domén

Štatistiky sa vypíšu aj s percentami pre lepšie pochopenie distribúcie dotazov.

### Príklad výstupu štatistík:
```
=== STATISTIKY ===
Celkovy pocet dotazov: 19
Blokovane dotazy: 6
Preposlane dotazy: 3
Ostatne typy dotazov (NOTIMP): 10
Presne zhody s blokovanymi domenami: 3
Zhody s poddomenami: 3

Percenta:
Blokovane: 31.6%
Preposlane: 15.8%
Ostatne typy (NOTIMP): 52.6%
==================
```

## Zoznam odovzdaných súborov

```
xbotlo01/
├── src/
│   ├── main.cpp              # Hlavný súbor programu
│   ├── DNSResolver.cpp/hpp   # Trieda pre DNS resolver
│   ├── DNSProtocol.cpp/hpp   # Spracovanie DNS protokolu
│   └── CLIParser.cpp/hpp     # Parser argumentov príkazového riadka
├── Makefile                  # Zostavenie projektu
├── simple_test.py            # Testovací skript
├── blocked_domains.txt       # Ukážkový súbor s blokovanými doménami
├── manual.pdf                # Dokumentácia
└── README.md                 # Tento súbor
```

