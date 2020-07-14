#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/time.h>

/* Inserite eventuali extern modules qui */

/* ************************************* */

enum { MAXLINES = 400 };

long long current_timestamp() {
    struct timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);
	/* te.tv_nsec nanoseconds divide by 1000 to get microseconds*/
	long long nanoseconds = tp.tv_sec*1000LL + tp.tv_nsec; // caculate nanoseconds  
    return nanoseconds;
}


int main(int argc, char *argv[]) {
    int i = 0;
    char bufferin[MAXLINES*8+1] ;
    char line[1024] ;
    long long tic_c, toc_c, tic_asm, toc_asm;

    char bufferout_c[MAXLINES*8+1] = "" ;
    char bufferout_asm[MAXLINES*8+1] = "" ;

    FILE *inputFile = fopen(argv[1], "r");

    if(argc != 3)
    {
        fprintf(stderr, "Syntax ./test <input_file> <output_file>\n");
        exit(1);
    }

    if (inputFile == 0)
    {
        fprintf(stderr, "failed to open the input file. Syntax ./test <input_file> <output_file>\n");
        exit(1);
    }

    while (i < MAXLINES && fgets(line, sizeof(line), inputFile))
    {
        i = i + 1;
        strcat( bufferin, line) ;
    }
	
    bufferin[MAXLINES*8] = '\0' ;

    fclose(inputFile);


    /* ELABORAZIONE in C */
    tic_c = current_timestamp();

  	int c = 0, j = 0;
  	int init, reset, ph, nck=0 ;
  	char st = '-', vlv[3] = "--", oldst = '-' ;
  	char tmpout[9] ;
  	char nckstr[3] = "--" ;

  	i = 0;


  	while ( bufferin[i] != '\0' ) {
		init = bufferin[i] - '0' ;
		reset = bufferin[i+2] - '0' ;
    	ph = (bufferin[i+4]-'0')*100 + (bufferin[i+5]-'0')*10 + (bufferin[i+6]-'0') ;

		strcpy(tmpout, "-,--,--\n") ;
      
		/* printf("i=%d, init: %d, reset: %d, ph: %d, tmpout: %s", i, init, reset, ph, tmpout) ; */
    	if ( init == 0 || reset == 1) {
          oldst = '-' ;
    	}
    	else
    	{
      		/* determino lo stato attuale */
      		if ( ph < 60 ) {
      			st = 'A' ;
      		}
      		else if ( ph >= 60 && ph <= 80 ) {
        		st = 'N' ;
      		}
      		else if ( ph > 80 ) {
        		st = 'B' ;
            }

      		/* aggiorno il contatore dei cicli */
      		if ( st != oldst ) {
        		nck = 0 ;
      		}
      		else
      		{
        		nck = nck + 1 ;
      		}
      		sprintf(nckstr,"%.2d",nck) ; 
			/* determino lo stato della valvola */
      		if ( st == 'A' && nck>4 ) {
        		strcpy(vlv,"BS") ;
			}
      		else if ( st == 'B' && nck>4 ) {
            strcpy(vlv,"AS") ;
            }
      		else {
            strcpy(vlv,"--") ;
            }
      		/* aggiorno oldst */
      		oldst = st ;
      		/* genero la stringa di output */
      		tmpout[0] = st ;
      		tmpout[2] = nckstr[0] ;
      		tmpout[3] = nckstr[1] ;
      		tmpout[5] = vlv[0] ;
      		tmpout[6] = vlv[1] ;
    	}

    	strcat( bufferout_c, tmpout) ;
    	i = i + 8 ;
  	}

    toc_c = current_timestamp();

  	long long c_time_in_nanos = toc_c - tic_c;

    /* FINE ELABORAZIONE C */


    /* INIZIO ELABORAZIONE ASM */

    tic_asm = current_timestamp();

    /* Assembly inline:
    Inserite qui il vostro blocco di codice assembly inline o richiamo a funzioni assembly.
    Il blocco di codice prende come input 'bufferin' e deve restituire una variabile stringa 'bufferout_asm' che verrà poi salvata su file. */

      __asm__(
       
       "inizio:"    
       "movl     %1, %%esi\n"                    //salvo l'indirizzo di bufferin in esi
       "leal     %0, %%edi\n"                    //salvo l'indirizzo di bufferout in edi
       "xorl     %%ebx,%%ebx\n"                  //resetto per sicurezza ebx(conterrà lo stato vecchio)
       "jmp      compare0\n"

       "incbuffer:"
       "addl     $8, %%edi\n"                    //scorro bufferin e bufferout
       "addl     $8, %%esi\n"

       "compare0:"
       "cmpl      $0, (%%esi)\n"                  //il codice ascii di \0 è 0
       "je        end\n"                          //se è uguale a 0 finisce l'elaborazione

       "riempi:"
       "movl     $0x2D2D2C2D, (%%edi)\n"          //riempio il bufferout con la stringa -,--,--\n
       "movl     $0xA2D2D2C, 4(%%edi)\n"
       
       "init_reset:"
       "movb      (%%esi), %%al\n"                //salvo init in al
       "cmpb      $48, %%al\n"                    //lo confronto con 0
       "je        resetmacchina\n"                
       "movb      2(%%esi), %%al\n"               //salvo reset in al
       "cmpb      $49, %%al\n"                    //lo confronto con 1
       "je        resetmacchina\n"
       "jmp       ph\n"

       "resetmacchina:"
       "xorl      %%ebx, %%ebx\n"                 //resetto il registro contenente il vecchio stato
       "jmp       incbuffer\n"                    //l'etichetta serve nel caso venga resettata la macchina dopo che è stata accesa

       "ph:"
       "movb      4(%%esi), %%al\n"               //confronto la decine del ph
       "cmpb      $48, %%al\n"                    //se è maggiore di 1 è sicuramente basico (ph 10)
       "jg        basico\n"
       "movb      5(%%esi), %%al\n"               //confronto l'unità del ph
       "cmpb      $56, %%al\n"                   
       "jg        basico\n"                       //se è maggiore di 8 è sicuramente basico
       "je        basneu\n"                       //se è uguale a 8 può essere basico o neutro
       "cmpb      $54, %%al\n"                    
       "jl        acido\n"                        //se è minore di 6 è sicuramente acido
       "jmp       neutro\n"                       //se è uguale a 6 è sicuremente neutro

       "basneu:"
       "movb      6(%%esi), %%al\n"               //confronto il decimo del ph
       "cmpb      $48, %%al\n"                    //se è maggiore di 0 è basico
       "jg        basico\n"                       
       "jmp       neutro\n"                       //se esegue questa jmp il valore è 80 (ph neutro)
   
       "basico:"
       "movb      $66, %%al\n"                    //salvo il carattere B(66) nel registro al
       "movb      %%al, (%%edi)\n"                //scrivo sul bufferout lo stato
       "jmp       nck\n"

       "acido:"
       "movb      $65, %%al\n"                    //salvo il carattere A(65) nel registro al
       "movb      %%al, (%%edi)\n"                //scrivo sul bufferout lo stato
       "jmp       nck\n"

       "neutro:"
       "movb      $78, %%al\n"                    //salvo il carattere N(78) nel registro al
       "movb      %%al, (%%edi)\n"                //scrivo sul bufferout lo stato
       "jmp       nck\n"
   
       "nck:"
       "cmpl      %%eax, %%ebx\n"                 //confronto lo stato attuale(eax) con lo stato vecchio(ebx)
       "jne       resnck\n"                       //se sono diversi vado a resettare nck
       "incb      %%ch\n"                         //se arriva qua sono uguali quindi incremento le unità di nck
       "cmpb      $58, %%ch\n"                    
       "je        incdecina\n"                    //se le unità sono pari a 10 vado a incrementare le decine
       "jmp       scrivinck\n"                    //altrimenti scrivo nck

       "incdecina:"
       "incb      %%cl\n"                         //incremento le decine
       "movb      $48, %%ch\n"                    //resetto le unità
       "jmp       scrivinck\n"

       "resnck:"
       "movw      $0x3030, %%cx\n"                //resetto nck 

       "scrivinck:"
       "movw      %%cx, 2(%%edi)\n"               //scrivo nck nel bufferout

       "salvastato:"
       "movl      %%eax, %%ebx\n"                 //il vecchio stato prende il valore dell'attuale

       "checkch:"
       "cmpb      $52, %%ch\n"                    //se nck è maggiore di 4 devo aprire le valvole quindi passo a controllare lo stato
       "jg        vlvcheck\n"

       "checkcl:"
       "cmpb      $48, %%cl\n"                    //controllo la decina (altrimenti al ciclo 13 non aprirebbe le valvole per esempio)
       "je        incbuffer\n"                    //se è uguale a 0 non devo aprire le valvole e vado al ciclo successivo

       "vlvcheck:"
       "cmpb      $65, %%al\n"                    
       "je        bsvlv\n"                        //se in al c'è il carattere A vado ad aprire le valvole BS
       "cmpb      $66, %%al\n"
       "je        asvlv\n"                        //se in al c'è il carattere B vado ad aprire le valvole AS 
       "jmp       incbuffer\n"                    //altrimenti è N e quindi vado al ciclo successivo(le valvole in N non si aprono)

       "bsvlv:"
       "movw      $0x5342, 5(%%edi)\n"            //metto i caratteri B ed S nelle rispettive posizioni del bufferout
       "jmp       incbuffer\n"                    //vado al ciclo successivo

       "asvlv:"
       "movw      $0x5341, 5(%%edi)\n"            //metto i caratteri A ed S nelle rispettive posizioni del bufferout
       "jmp       incbuffer\n"                    //vado al ciclo successivo

       "end: \n"                                  //se arrivo qua ho finito l'elaborazione



       :"=g" (bufferout_asm)    //[0]output
       :"g" (bufferin)          //[1]input
       :"%eax", "%ebx", "%ecx"

       );

    toc_asm = current_timestamp();

  	long long asm_time_in_nanos = toc_asm - tic_asm;

    /* FINE ELABORAZIONE ASM */


    printf("C time elapsed: %lld ns\n", c_time_in_nanos);
    printf("ASM time elapsed: %lld ns\n", asm_time_in_nanos);

    /* Salvataggio dei risultati ASM */
  	FILE *outputFile;
    outputFile = fopen (argv[2], "w");
    fprintf (outputFile, "%s", bufferout_asm);
    fclose (outputFile);

    return 0;
}
