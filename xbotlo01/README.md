# Filtrujúci DNS resolver

**Autor:** Filip Botlo (xbotlo01)  
**Dátum vytvorenia:** 2025

## Popis programu

Program implementuje filtrujúci DNS resolver, ktorý blokuje dotazy typu A na domény zo zadaného zoznamu a ich poddomény. Ostatné dotazy prepúšťa v nezmenenej podobe špecifikovanému resolveru.

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
- **`-v`** - Verbose mód - vypisuje jednoduché informácie o dotazoch
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

## Verbose mód

Pri použití argumentu `-v` program vypisuje jednoduché informácie o každom dotaze v angličtine:

- `query: domain.com, action: forwarded` - dotaz bol preposlaný resolveru
- `query: domain.com, action: blocked` - dotaz bol zablokovaný
- `query: domain.com, action: notimp` - typ dotazu nie je implementovaný (iba A records)

Pri spustení sa vypíše:
- `dns filter started on port: X`
- `resolver: X`

## Štatistiky

Pri použití argumentu `--stats` program po stlačení Ctrl+C vypíše nasledujúce štatistiky:

- **total queries** - celkový počet prijatých DNS dotazov
- **blocked** - počet dotazov, ktoré boli zablokované (REFUSED)
- **forwarded** - počet dotazov, ktoré boli preposlané resolveru
- **notimp** - počet dotazov s nepodporovanými typmi (AAAA, MX, TXT, NS, CNAME, atď.)
- **blocked - exact matches** - počet presných zhôd s blokovanými doménami (napr. `example.com` == `example.com`)
- **blocked - subdomain matches** - počet zhôd s poddomenami blokovaných domén (napr. `www.example.com` je poddomena `example.com`)

**Poznámka:** `blocked - exact matches` + `blocked - subdomain matches` ≤ `blocked`, pretože niektoré blokované dotazy môžu byť klasifikované inak.

### Príklad výstupu štatistík:
```
total queries: 14
blocked: 6
forwarded: 6
notimp: 3
blocked - exact matches: 3
blocked - subdomain matches: 3
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

