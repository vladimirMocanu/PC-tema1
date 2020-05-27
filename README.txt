Mocanu-Prejma Vladimir-Gerogian
325CB
			Tema1 - Protocol cu ferestra glisanta

Avem doua fisiere send.c si recv.c . O sa folosim o structura care contine un
char unde o sa punem datele, int criteriu si int corrupt. Am luat functia
in_cksum din laboratorul 5 pentru a vedea daca datele sunt corupte sau nu. 
Ideea de implementare: Folosim doi vectori de tipul package unde retinem pachetele
trimise sau primite si inca doi vectori de tipul int unde vedem daca s-au trimis 
pachetele.

send.c: Deschidem fisierul cu functia open, si luam paramentri speed si delay.
Calculam BDP si count(marimea ferestrei). Cu functia fstat aflam dimensiunea fisierului
si o trimiteam mai departe la recv impreuna cu delay. Cat timp (size > 0 ) citim din fisier
si trimitem pachete pana umplem fereastra. Dupa ce am umplut fereastra avem un recv_message
si asteptam raspunsul. Cand primim un ACK citiim din nou din fisier si trimitem pachetul.
Astfel, fereastra ramane mereu plina. Dupa ce citim din fisier asteptam si restul de ACK-uri
si eventual trimitem pachetele care s-au pierdut sau sunt corupte. Cand primim raspunsul Scris
inchidem fisierul si iesim din program.

recv.c: Deschidem fisierul cu functia open, si asteptam sa primim pachetul de la send.c 
cu size si delay. Cat timp primim pachete de la send.c pe copiem in buffer, verificam daca
este corup pachetul si trimitem ACK cu criteriu pachetului. Astfel, stim ca l-am primit.
Cand nu mai primim pachete, iesim din while. Avem din nou un while(size > 0). Parcurgem
vectorul buff, si daca buff[i] este mai mic ca 0 inseamna ca nu am primit acel pachet sau 
este corupt. Trimitem raspuns la send.c cu mesajul Gresit si asteptam sa primim inapoi
pachetul bun. Daca buff[i] este mai mare ca 0 scriem in fisier. Cand termina iesim din 
while, trimitem la send.c mesajul Scris, inchidem fisierul si iesim din program.

