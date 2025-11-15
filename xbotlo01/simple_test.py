#!/usr/bin/env python3 
import subprocess
import time
import os

test_results = []

def run_test(test_name, command, expected_exit_code=0):
    print(f"\nTest: {test_name}")
    print(f"Prikaz: {' '.join(command)}")
    
    try:
        result = subprocess.run(command, capture_output=True, text=True, timeout=10)
        success = result.returncode == expected_exit_code
        status = "PASSED" if success else "FAILED"
        print(f"{status} (exit code: {result.returncode})")
        
        if not success and result.stderr:
            print(f"   Chyba: {result.stderr.strip()}")
        
        test_results.append({
            'name': test_name,
            'passed': success,
            'command': ' '.join(command),
            'exit_code': result.returncode,
            'error': result.stderr.strip() if result.stderr else None
        })
        
        return success
    except subprocess.TimeoutExpired:
        print("FAILED (timeout)")
        test_results.append({
            'name': test_name,
            'passed': False,
            'command': ' '.join(command),
            'exit_code': -1,
            'error': 'Timeout'
        })
        return False
    except Exception as e:
        print(f"FAILED (exception: {e})")
        test_results.append({
            'name': test_name,
            'passed': False,
            'command': ' '.join(command),
            'exit_code': -1,
            'error': str(e)
        })
        return False

def test_argument_order(test_name, command):
    print(f"\nTest: {test_name}")
    print(f"Prikaz: {' '.join(command)}")
    
    dns_process = None
    try:
        dns_process = subprocess.Popen(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        
        time.sleep(2)
        
        if dns_process.poll() is None:
            print("DNS filter sa spustil a bezi")
            test_results.append({
                'name': test_name,
                'passed': True,
                'command': ' '.join(command),
                'exit_code': 0,
                'error': None
            })
            return True
        else:
            stdout, stderr = dns_process.communicate()
            error_msg = stderr.decode('utf-8').strip()
            
            if "IPv6" in test_name and ("network unreachable" in error_msg or "Failed to connect" in error_msg):
                print(f"IPv6 nie je dostupny - ignoruje sa test")
                test_results.append({
                    'name': test_name,
                    'passed': True,
                    'command': ' '.join(command),
                    'exit_code': 0,
                    'error': 'IPv6 nie je dostupný'
                })
                return True
            else:
                print(f"DNS filter sa nespustil (exit code: {dns_process.returncode})")
                if error_msg:
                    print(f"   Chyba: {error_msg}")
                test_results.append({
                    'name': test_name,
                    'passed': False,
                    'command': ' '.join(command),
                    'exit_code': dns_process.returncode,
                    'error': error_msg or 'DNS filter sa nespustil'
                })
                return False
            
    except Exception as e:
        print(f"Chyba: {e}")
        test_results.append({
            'name': test_name,
            'passed': False,
            'command': ' '.join(command),
            'exit_code': -1,
            'error': str(e)
        })
        return False
    finally:
        if dns_process and dns_process.poll() is None:
            dns_process.terminate()
            try:
                dns_process.wait(timeout=2)
            except:
                dns_process.kill()

def test_dns_query(domain, port, expected_status="REFUSED"):
    test_name = f"DNS Query: {domain}"
    print(f"\nDNS Test: {domain}")
    
    try:
        result = subprocess.run(
            ["dig", "@127.0.0.1", "-p", str(port), domain],
            capture_output=True,
            text=True,
            timeout=10
        )
        
        print(f"Dig vystup pre {domain}:")
        print(f"STDOUT: {result.stdout}")
        print(f"STDERR: {result.stderr}")
        print(f"Return code: {result.returncode}")
        
        status = None
        for line in result.stdout.split('\n'):
            if 'status:' in line:
                status = line.split('status:')[1].strip().split(',')[0]
                break
        
        if status:
            success = status == expected_status
            status_icon = "PASSED" if success else "FAILED"
            print(f"{status_icon} {domain} -> {status} (ocakavane: {expected_status})")
        else:
            print(f"FAILED {domain} -> Status nenajdeny v dig vystupe")
            status = "UNKNOWN"
            success = False
        
        test_results.append({
            'name': test_name,
            'passed': success,
            'command': f"dig @127.0.0.1 -p {port} {domain}",
            'exit_code': 0,
            'error': f"Očakávané: {expected_status}, Skutočné: {status}" if not success else None
        })
        
        return success
        
    except Exception as e:
        print(f"FAILED {domain} -> Chyba: {e}")
        test_results.append({
            'name': test_name,
            'passed': False,
            'command': f"dig @127.0.0.1 -p {port} {domain}",
            'exit_code': -1,
            'error': str(e)
        })
        return False

def main():
    print("DNS Filter - Jednoduchy Test")
    print("=" * 40)
    
    print("\nKompilovanie...")
    if not run_test("Kompilacia", ["make", "clean"]):
        return False
    if not run_test("Kompilacia", ["make"]):
        return False
    
    print("\nTesty argumentov:")
    run_test("Help", ["./dns", "--help"], 0)
    run_test("Ziadne argumenty", ["./dns"], 1)
    run_test("Chyba -s", ["./dns", "-f", "blocked_domains.txt"], 1)
    run_test("Chyba -f", ["./dns", "-s", "8.8.8.8"], 1)
    run_test("Neplatny port", ["./dns", "-s", "8.8.8.8", "-f", "blocked_domains.txt", "-p", "-1"], 1)
    
    print("\nTesty roznych poradi argumentov:")
    
    test_argument_order("Poradie: -s -f -p", ["./dns", "-s", "8.8.8.8", "-f", "blocked_domains.txt", "-p", "5350"])
    test_argument_order("Poradie: -f -s -p", ["./dns", "-f", "blocked_domains.txt", "-s", "8.8.8.8", "-p", "5351"])
    test_argument_order("Poradie: -p -s -f", ["./dns", "-p", "5352", "-s", "8.8.8.8", "-f", "blocked_domains.txt"])
    test_argument_order("Poradie: -s -p -f -v", ["./dns", "-s", "8.8.8.8", "-p", "5370", "-f", "blocked_domains.txt", "-v"])
    test_argument_order("Poradie: -v -f -s -p", ["./dns", "-v", "-f", "blocked_domains.txt", "-s", "8.8.8.8", "-p", "5371"])
    
    print("\nTesty IPv6 adries:")
    test_argument_order("IPv6 DNS server", ["./dns", "-s", "2001:4860:4860::8888", "-f", "blocked_domains.txt", "-p", "5355"])
    test_argument_order("IPv6 localhost", ["./dns", "-s", "::1", "-f", "blocked_domains.txt", "-p", "5356"])
    test_argument_order("IPv6 s portom", ["./dns", "-s", "2001:4860:4860::8844", "-f", "blocked_domains.txt", "-p", "5357"])
    
    print("\nTesty domenovych mien DNS servera:")
    test_argument_order("Google DNS", ["./dns", "-s", "dns.google", "-f", "blocked_domains.txt", "-p", "5358"])
    test_argument_order("Cloudflare DNS", ["./dns", "-s", "1.1.1.1", "-f", "blocked_domains.txt", "-p", "5359"])
    test_argument_order("Quad9 DNS", ["./dns", "-s", "9.9.9.9", "-f", "blocked_domains.txt", "-p", "5360"])
    
    print("\nTesty neplatnych argumentov:")
    run_test("Neplatny port 65536", ["./dns", "-s", "8.8.8.8", "-f", "blocked_domains.txt", "-p", "65536"], 1)
    run_test("Neplatny port abc", ["./dns", "-s", "8.8.8.8", "-f", "blocked_domains.txt", "-p", "abc"], 1)
    run_test("Neznamy argument -x", ["./dns", "-s", "8.8.8.8", "-f", "blocked_domains.txt", "-x"], 1)
    run_test("Duplicitny -s", ["./dns", "-s", "8.8.8.8", "-s", "1.1.1.1", "-f", "blocked_domains.txt"], 1)
    run_test("Duplicitny -f", ["./dns", "-s", "8.8.8.8", "-f", "test1.txt", "-f", "test2.txt"], 1)
    run_test("Duplicitny -p", ["./dns", "-s", "8.8.8.8", "-f", "blocked_domains.txt", "-p", "5350", "-p", "5351"], 1)
    
    print("\nTesty platnych portov:")
    test_argument_order("Port 0", ["./dns", "-s", "8.8.8.8", "-f", "blocked_domains.txt", "-p", "0"])
    test_argument_order("Port 1024", ["./dns", "-s", "8.8.8.8", "-f", "blocked_domains.txt", "-p", "1024"])
    test_argument_order("Port 8080", ["./dns", "-s", "8.8.8.8", "-f", "blocked_domains.txt", "-p", "8080"])
    
    print("\nTesty edge cases:")
    run_test("Prazdny -s", ["./dns", "-s", "", "-f", "blocked_domains.txt"], 1)
    run_test("Prazdny -f", ["./dns", "-s", "8.8.8.8", "-f", ""], 1)
    run_test("Len medzery v -s", ["./dns", "-s", "   ", "-f", "blocked_domains.txt"], 1)
    run_test("Len medzery v -f", ["./dns", "-s", "8.8.8.8", "-f", "   "], 1)
    run_test("Velmi dlhá IP", ["./dns", "-s", "1.2.3.4.5.6.7.8.9.10", "-f", "blocked_domains.txt"], 1)
    run_test("Neplatná IPv6", ["./dns", "-s", "2001:4860:4860::8888::", "-f", "blocked_domains.txt"], 1)
    
    print("\nTesty neplatnych suborov:")
    run_test("Neexistujuci subor", ["./dns", "-s", "8.8.8.8", "-f", "nonexistent.txt", "-p", "5361"], 1)
    run_test("Prazdny subor", ["./dns", "-s", "8.8.8.8", "-f", "test_empty.txt", "-p", "5362"], 1)
    run_test("Subor len s komentarmi", ["./dns", "-s", "8.8.8.8", "-f", "test_only_comments.txt", "-p", "5363"], 1)
    run_test("Subor bez opravneni", ["./dns", "-s", "8.8.8.8", "-f", "/root/restricted.txt", "-p", "5364"], 1)
    
    print("\nTesty chybovych DNS serverov:")
    run_test("Neplatny DNS server", ["./dns", "-s", "999.999.999.999", "-f", "blocked_domains.txt", "-p", "5365"], 1)
    run_test("Neexistujuci DNS server", ["./dns", "-s", "nonexistent.example", "-f", "blocked_domains.txt", "-p", "5366"], 1)
    run_test("Prazdny DNS server", ["./dns", "-s", "", "-f", "blocked_domains.txt", "-p", "5367"], 1)
    
    print("\n🔌 Testy chybových portov:")
    run_test("Port 65536", ["./dns", "-s", "8.8.8.8", "-f", "blocked_domains.txt", "-p", "65536"], 1)
    run_test("Port -1", ["./dns", "-s", "8.8.8.8", "-f", "blocked_domains.txt", "-p", "-1"], 1)
    run_test("Port abc", ["./dns", "-s", "8.8.8.8", "-f", "blocked_domains.txt", "-p", "abc"], 1)
    
    print("\nVytvaranie testovacich suborov...")
    
    with open("test_domains.txt", "w") as f:
        f.write("example.com\n")
        f.write("facebook.com\n")
        f.write("youtube.com\n")
    
    with open("test_comments_linux.txt", "w") as f:
        f.write("# Komentár na začiatku\n")
        f.write("example.com\n")
        f.write("\n")  
        f.write("facebook.com\n")
        f.write("# Ďalší komentár\n")
        f.write("youtube.com\n")
        f.write("  github.com  \n")  
    
    with open("test_windows.txt", "w") as f:
        f.write("example.com\r\n")
        f.write("facebook.com\r\n")
        f.write("youtube.com\r\n")
    
    with open("test_mac.txt", "w") as f:
        f.write("example.com\r")
        f.write("facebook.com\r")
        f.write("youtube.com\r")
    
    with open("test_empty.txt", "w") as f:
        pass
    
    with open("test_only_comments.txt", "w") as f:
        f.write("# Len komentáre\n")
        f.write("# Žiadne domény\n")
    
    with open("test_edge_cases.txt", "w") as f:
        f.write("example.com\n")
        f.write("facebook.com\n")
        f.write("youtube.com\n")
        f.write("sub.example.com\n")  
        f.write("www.facebook.com\n")  
        f.write("EXAMPLE.COM\n")  
        f.write("Facebook.Com\n")  
        f.write("  youtube.com  \n")  
        f.write("\tfacebook.com\t\n")  
        f.write("# Komentár\n")
        f.write("\n")  
        f.write("github.com\n")
    
    print("\nSpustanie DNS filtra...")
    dns_process = None
    try:
        dns_process = subprocess.Popen(
            ["./dns", "-s", "8.8.8.8", "-f", "test_domains.txt", "-p", "5350", "-v"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        
        time.sleep(5)
        
        if dns_process.poll() is not None:
            print("DNS filter sa nespustil")
            stdout, stderr = dns_process.communicate()
            print(f"STDOUT: {stdout.decode('utf-8').strip()}")
            print(f"STDERR: {stderr.decode('utf-8').strip()}")
            return False
        
        import socket
        test_port = 5350
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.settimeout(1)
            sock.connect(('127.0.0.1', test_port))
            sock.close()
            print(f"Port {test_port} je dostupny")
        except:
            print(f"Port {test_port} nie je dostupny")
            dns_process.terminate()
            return False
        
        print("DNS filter bezi")
        
        print("\nTesty DNS dotazov:")
        test_dns_query("example.com", 5350, "REFUSED")
        test_dns_query("facebook.com", 5350, "REFUSED")
        test_dns_query("youtube.com", 5350, "REFUSED")
        test_dns_query("google.com", 5350, "NOERROR")
        test_dns_query("github.com", 5350, "NOERROR")
        test_dns_query("www.example.com", 5350, "REFUSED")
        test_dns_query("www.google.com", 5350, "NOERROR")
        
        print("\nTesty case sensitivity:")
        test_dns_query("EXAMPLE.COM", 5350, "REFUSED")
        test_dns_query("Facebook.Com", 5350, "REFUSED")
        
        print("\nTesty subdomen:")
        test_dns_query("sub.example.com", 5350, "REFUSED")
        test_dns_query("www.facebook.com", 5350, "REFUSED")
        test_dns_query("mail.youtube.com", 5350, "REFUSED")
        test_dns_query("www.google.com", 5350, "NOERROR")
        test_dns_query("mail.google.com", 5350, "NOERROR")
        
        print("\nTesty roznych formatov suborov:")
        
        print("\nTest: Subor s komentarmi a medzerami")
        dns_process_comments = subprocess.Popen(
            ["./dns", "-s", "8.8.8.8", "-f", "test_comments_linux.txt", "-p", "5353", "-v"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        time.sleep(2)
        if dns_process_comments.poll() is None:
            print("DNS filter s komentarmi bezi")
            test_dns_query("example.com", 5353, "REFUSED")
            test_dns_query("facebook.com", 5353, "REFUSED")
            test_dns_query("youtube.com", 5353, "REFUSED")
            test_dns_query("github.com", 5353, "NOERROR")  
            test_dns_query("GOOGLE.COM", 5353, "NOERROR")  
            test_dns_query("google.com", 5353, "NOERROR")
            dns_process_comments.terminate()
            try:
                dns_process_comments.wait(timeout=5)
            except:
                dns_process_comments.kill()
        else:
            print("DNS filter s komentarmi sa nespustil")
            stdout, stderr = dns_process_comments.communicate()
            print(f"STDOUT: {stdout.decode('utf-8').strip()}")
            print(f"STDERR: {stderr.decode('utf-8').strip()}")
        
        print("\nTest: Windows konce riadkov")
        dns_process_windows = subprocess.Popen(
            ["./dns", "-s", "8.8.8.8", "-f", "test_windows.txt", "-p", "5372", "-v"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        time.sleep(2)
        if dns_process_windows.poll() is None:
            print("DNS filter s Windows koncami bezi")
            test_dns_query("example.com", 5372, "REFUSED")
            test_dns_query("facebook.com", 5372, "REFUSED")
            test_dns_query("youtube.com", 5372, "REFUSED")
            test_dns_query("google.com", 5372, "NOERROR")
            dns_process_windows.terminate()
            try:
                dns_process_windows.wait(timeout=5)
            except:
                dns_process_windows.kill()
        else:
            print("DNS filter s Windows koncami sa nespustil")
            stdout, stderr = dns_process_windows.communicate()
            print(f"STDOUT: {stdout.decode('utf-8').strip()}")
            print(f"STDERR: {stderr.decode('utf-8').strip()}")
        
        print("\nTest: Mac konce riadkov")
        dns_process_mac = subprocess.Popen(
            ["./dns", "-s", "8.8.8.8", "-f", "test_mac.txt", "-p", "5373", "-v"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        time.sleep(2)
        if dns_process_mac.poll() is None:
            print("PASSED DNS filter s Mac koncami beží")
            test_dns_query("example.com", 5373, "REFUSED")
            test_dns_query("facebook.com", 5373, "REFUSED")
            test_dns_query("youtube.com", 5373, "REFUSED")
            test_dns_query("google.com", 5373, "NOERROR")
            dns_process_mac.terminate()
            try:
                dns_process_mac.wait(timeout=5)
            except:
                dns_process_mac.kill()
        else:
            print("FAILED DNS filter s Mac koncami sa nespustil")
            stdout, stderr = dns_process_mac.communicate()
            print(f"STDOUT: {stdout.decode('utf-8').strip()}")
            print(f"STDERR: {stderr.decode('utf-8').strip()}")
        
        print("\n Test: Edge cases subor")
        dns_process_edge = subprocess.Popen(
            ["./dns", "-s", "8.8.8.8", "-f", "test_edge_cases.txt", "-p", "5356", "-v"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        time.sleep(2)
        if dns_process_edge.poll() is None:
            print("PASSED DNS filter s edge cases beží")
            test_dns_query("sub.example.com", 5356, "REFUSED")
            test_dns_query("www.facebook.com", 5356, "REFUSED")
            test_dns_query("EXAMPLE.COM", 5356, "REFUSED")
            test_dns_query("Facebook.Com", 5356, "REFUSED")
            test_dns_query("youtube.com", 5356, "REFUSED")  
            test_dns_query("facebook.com", 5356, "REFUSED")  
            test_dns_query("github.com", 5356, "REFUSED")
            test_dns_query("google.com", 5356, "NOERROR")
            dns_process_edge.terminate()
            try:
                dns_process_edge.wait(timeout=5)
            except:
                dns_process_edge.kill()
        else:
            print("FAILED DNS filter s edge cases sa nespustil")
            stdout, stderr = dns_process_edge.communicate()
            print(f"STDOUT: {stdout.decode('utf-8').strip()}")
            print(f"STDERR: {stderr.decode('utf-8').strip()}")
        
        print("\n Testy roznych query typov:")
        
        query_types = [
            ("AAAA", "IPv6 address"),
            ("MX", "Mail exchange"),
            ("TXT", "Text record"),
            ("CNAME", "Canonical name"),
            ("NS", "Name server"),
            ("SOA", "Start of authority"),
            ("PTR", "Pointer record"),
            ("SRV", "Service record")
        ]
        
        for query_type, description in query_types:
            try:
                result = subprocess.run(
                    ["dig", "@127.0.0.1", "-p", "5350", "google.com", query_type],
                    capture_output=True,
                    text=True,
                    timeout=5
                )
                success = False
                for line in result.stdout.split('\n'):
                    if 'status:' in line:
                        status = line.split('status:')[1].strip().split(',')[0]
                        success = status == "NOTIMP"
                        status_icon = "PASSED" if success else "FAILED"
                        print(f"{status_icon} {query_type} query ({description}) -> {status} (očakávané: NOTIMP)")
                        break
                
                test_results.append({
                    'name': f'{query_type} Query Test',
                    'passed': success,
                    'command': f'dig @127.0.0.1 -p 5350 google.com {query_type}',
                    'exit_code': 0,
                    'error': f"Očakávané: NOTIMP, Skutočné: {status}" if not success else None
                })
            except Exception as e:
                print(f"FAILED {query_type} query -> Chyba: {e}")
                test_results.append({
                    'name': f'{query_type} Query Test',
                    'passed': False,
                    'command': f'dig @127.0.0.1 -p 5350 google.com {query_type}',
                    'exit_code': -1,
                    'error': str(e)
                })
        
        print("\n Test A query (povolena):")
        try:
            result = subprocess.run(
                ["dig", "@127.0.0.1", "-p", "5350", "google.com", "A"],
                capture_output=True,
                text=True,
                timeout=5
            )
            success = False
            for line in result.stdout.split('\n'):
                if 'status:' in line:
                    status = line.split('status:')[1].strip().split(',')[0]
                    success = status == "NOERROR"
                    status_icon = "PASSED" if success else "FAILED"
                    print(f"{status_icon} A query -> {status} (očakávané: NOERROR)")
                    break
            
            test_results.append({
                'name': 'A Query Test (povolena)',
                'passed': success,
                'command': 'dig @127.0.0.1 -p 5350 google.com A',
                'exit_code': 0,
                'error': f"Očakávané: NOERROR, Skutočné: {status}" if not success else None
            })
        except Exception as e:
            print(f"FAILED A query -> Chyba: {e}")
            test_results.append({
                'name': 'A Query Test (povolena)',
                'passed': False,
                'command': 'dig @127.0.0.1 -p 5350 google.com A',
                'exit_code': -1,
                'error': str(e)
            })
        
        print("\n Test IPv6 DNS server:")
        try:
            dns_process_ipv6 = subprocess.Popen(
                ["./dns", "-s", "2001:4860:4860::8888", "-f", "test_domains.txt", "-p", "5351", "-v"],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE
            )
            time.sleep(3)
            
            if dns_process_ipv6.poll() is None:
                print("PASSED IPv6 DNS filter beží")
                test_dns_query("example.com", 5351, "REFUSED")
                test_dns_query("google.com", 5351, "NOERROR")
                dns_process_ipv6.terminate()
                try:
                    dns_process_ipv6.wait(timeout=5)
                except:
                    dns_process_ipv6.kill()
            else:
                print("FAILED IPv6 DNS filter sa nespustil")
                stdout, stderr = dns_process_ipv6.communicate()
                print(f"STDOUT: {stdout.decode('utf-8').strip()}")
                print(f"STDERR: {stderr.decode('utf-8').strip()}")
                test_results.append({
                    'name': 'IPv6 DNS Server Test',
                    'passed': False,
                    'command': './dns -s 2001:4860:4860::8888 -f test_domains.txt -p 5351 -v',
                    'exit_code': dns_process_ipv6.returncode,
                    'error': 'IPv6 DNS filter sa nespustil'
                })
        except Exception as e:
            print(f"FAILED IPv6 DNS test -> Chyba: {e}")
            test_results.append({
                'name': 'IPv6 DNS Server Test',
                'passed': False,
                'command': './dns -s 2001:4860:4860::8888 -f test_domains.txt -p 5351 -v',
                'exit_code': -1,
                'error': str(e)
            })
        
        print("\n Test doménového mena DNS servera:")
        try:
            dns_process_domain = subprocess.Popen(
                ["./dns", "-s", "dns.google", "-f", "test_domains.txt", "-p", "5352", "-v"],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE
            )
            time.sleep(3)
            
            if dns_process_domain.poll() is None:
                print("PASSED DNS filter s domenovym menom beží")
                test_dns_query("example.com", 5352, "REFUSED")
                test_dns_query("google.com", 5352, "NOERROR")
                dns_process_domain.terminate()
                try:
                    dns_process_domain.wait(timeout=5)
                except:
                    dns_process_domain.kill()
            else:
                print("FAILED DNS filter s domenovym menom sa nespustil")
                stdout, stderr = dns_process_domain.communicate()
                print(f"STDOUT: {stdout.decode('utf-8').strip()}")
                print(f"STDERR: {stderr.decode('utf-8').strip()}")
                test_results.append({
                    'name': 'Domain Name DNS Server Test',
                    'passed': False,
                    'command': './dns -s dns.google -f test_domains.txt -p 5352 -v',
                    'exit_code': dns_process_domain.returncode,
                    'error': 'DNS filter s domenovym menom sa nespustil'
                })
        except Exception as e:
            print(f"FAILED Domain name DNS test -> Chyba: {e}")
            test_results.append({
                'name': 'Domain Name DNS Server Test',
                'passed': False,
                'command': './dns -s dns.google -f test_domains.txt -p 5352 -v',
                'exit_code': -1,
                'error': str(e)
            })
        
        print("\n Vsetky testy dokoncene!")
        
    finally:
        if dns_process:
            print("\n Zastavovanie DNS filtra...")
            dns_process.terminate()
            try:
                dns_process.wait(timeout=5)
            except:
                dns_process.kill()
        
        cleanup_files = [
            "test_domains.txt", "test_comments_linux.txt", "test_windows.txt", 
            "test_mac.txt", "test_empty.txt", "test_only_comments.txt", "test_edge_cases.txt"
        ]
        for file in cleanup_files:
            try:
                os.remove(file)
            except:
                pass
    
    print("\n" + "="*60)
    print(" VÝSLEDKY TESTOV")
    print("="*60)
    
    passed_count = sum(1 for test in test_results if test['passed'])
    failed_count = len(test_results) - passed_count
    
    print(f"Celkový počet testov: {len(test_results)}")
    print(f"PASSED Uspesne: {passed_count}")
    print(f"FAILED: {failed_count}")
    print(f"Uspesnost: {(passed_count/len(test_results))*100:.1f}%")
    
    if failed_count > 0:
        print("\n" + "="*60)
        print("FAILED TESTY")
        print("="*60)
        
        failed_tests = [test for test in test_results if not test['passed']]
        for i, test in enumerate(failed_tests, 1):
            print(f"\n{i}. {test['name']}")
            print(f"   Príkaz: {test['command']}")
            if test['error']:
                print(f"   Chyba: {test['error']}")
    
    if failed_count == 0:
        print("\n Vsetky testy presli!")
        return True
    else:
        print(f"\n {failed_count} testov zlyhalo!")
        return False

if __name__ == "__main__":
    main()
