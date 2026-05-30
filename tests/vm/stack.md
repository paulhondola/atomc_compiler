================================================================================
  STIVA VM — CADRUL FUNCTIEI SI CODUL GENERAT
  Exemplu: testgc.c — put_i(fact(3))  =>  6
================================================================================

CONVENTII:
  SP  = Stack Pointer (varful stivei, adresa celui mai recent element)
  FP  = Frame Pointer (pointeaza spre celula old_FP din cadrul curent)
  [n] = offsetul n fata de FP (ex: FP[-2] = doua celule inainte de FP)
  ^   = indicator spre adresa curenta a SP sau FP
  Val = union{int i; double f; void *p; ...}  — dimensiune uniforma pe stiva

================================================================================
  PASUL 0 — Inainte de apelul main
================================================================================

  entryCode:
    CALL -> main->fn.instr    // push ret = &HALT, jump la ENTER din main
    HALT

  Stiva inainte de CALL (goala):
    (goala)

  CALL face: pushp(IP->next)   =>  pune adresa instructiunii HALT pe stiva
             IP = main->fn.instr

  Stiva dupa CALL (inainte de ENTER din main):
    stack[0] = &HALT            <-- SP   (aceasta e adresa de intoarcere)

================================================================================
  PASUL 1 — ENTER 2  (prima instructiune din main)
================================================================================

  main are: 0 parametri, 2 variabile locale (r la varIdx=0, i la varIdx=1)

  ENTER face:
    1. pushp(FP)         =>  FP era NULL initial (sau pointer din cadru anterior)
    2. FP = SP           =>  FP pointeaza spre celula unde am pus old_FP
    3. SP += 2           =>  aloca 2 locali (nu ii initializeaza, doar avanseaza SP)

  Stiva dupa ENTER 2:

    index   continut            relativ la FP
    ------  ------------------  --------------
    [0]     &HALT               FP[-1]   <-- adresa de intoarcere (pusa de CALL)
    [1]     NULL (old_FP)       FP[ 0]   <-- FP pointeaza AICI
    [2]     (neinitializat: r)  FP[ 1]   <-- variabila locala r (int)
    [3]     (neinitializat: i)  FP[ 2]   <-- variabila locala i (int)
                                             ^SP

  Nota: main nu are parametri => nu exista FP[-2], FP[-3] etc. (nu s-au push-uit argumente)
  Nota: FP[-1] = ret_addr = &HALT (pus de CALL din entryCode)

================================================================================
  PASUL 2 — put_i(4.9)
================================================================================

  Cod generat:
    PUSH.f 4.9
    CONV.f.i          // (int)4.9 = 4
    CALL_EXT put_i    // put_i() face popi() => afiseaza "=> 4"

  Stiva in timpul executiei (dupa ENTER, inainte de apel):

    [0]  &HALT         FP[-1]
    [1]  old_FP        FP[ 0]  <-- FP
    [2]  r (?)         FP[ 1]
    [3]  i (?)         FP[ 2]  <-- SP (locala i alocata, SP avansat)

  Dupa PUSH.f 4.9:
    [4]  4.9 (double)          <-- SP

  Dupa CONV.f.i:
    [4]  4 (int)               <-- SP   // double 4.9 convertit la int 4

  CALL_EXT put_i:
    put_i() face popi()  =>  pop 4 de pe stiva, afiseaza "=> 4"
    Stiva revine la SP = &[3] (= FP[2])

================================================================================
  PASUL 3 — put_i(fact(3))
================================================================================

  Cod generat in main:
    PUSH.i 3           // argumentul pentru fact
    CALL fact          // push ret_addr_1, jump la ENTER din fact

  --- Stiva inainte de CALL fact ---
    [0]  &HALT         FP_main[-1]
    [1]  old_FP_0      FP_main[ 0]  <-- FP_main
    [2]  r (?)         FP_main[ 1]
    [3]  i (?)         FP_main[ 2]
    [4]  3 (int)                    <-- SP (argumentul n=3)

  CALL fact face: pushp(IP->next)  =>  push adresa instructiunii dupa CALL (ret_addr_1)
                  IP = fact->fn.instr (= ENTER 0)

  --- Stiva dupa CALL, inainte de ENTER ---
    [0]  &HALT         FP_main[-1]
    [1]  old_FP_0      FP_main[ 0]  <-- FP_main
    [2]  r (?)         FP_main[ 1]
    [3]  i (?)         FP_main[ 2]
    [4]  3             (argumentul n)
    [5]  ret_addr_1                 <-- SP

================================================================================
  PASUL 4 — ENTER 0  (prima instructiune din fact)
================================================================================

  fact are: 1 parametru (n, paramIdx=0), 0 variabile locale

  ENTER 0 face:
    1. pushp(FP_main)   =>  salveaza FP_main
    2. FP = SP          =>  FP_fact pointeaza spre old_FP_main
    3. SP += 0          =>  niciun local de alocat

  --- Stiva dupa ENTER 0 ---

    index   continut            relativ la FP_fact
    ------  ------------------  -------------------
    [0]     &HALT               (in jos, sub frame-ul main)
    [1]     old_FP_0            (in jos)
    [2]     r (?)               (in jos)
    [3]     i (?)               (in jos)
    [4]     3                   FP_fact[-2]  <-- param n = 3
    [5]     ret_addr_1          FP_fact[-1]  <-- adresa de intoarcere
    [6]     FP_main             FP_fact[ 0]  <-- FP_fact pointeaza AICI, SP la [6]
                                                 ^FP_fact, ^SP

  Cadrul functiei fact (relativ la FP_fact):
    FP_fact[-2] = n = 3         (singurul parametru)
    FP_fact[-1] = ret_addr_1    (adresa instructiunii dupa CALL fact in main)
    FP_fact[ 0] = FP_main       (frame pointer salvat al apelantului)
    (niciun local: SP = FP_fact)

================================================================================
  PASUL 5 — Corpul lui fact(3): if(n < 3)
================================================================================

  Cod generat pentru if(n < 3):
    FPADDR.i -2        // push &FP_fact[-2].i = adresa lui n
    LOAD.i             // pop addr, push *(int*)addr = 3
    PUSH.i 3           // push 3
    LESS.i             // pop 3 si 3, push (3 < 3) = 0
    JF [else_label]    // 0 = fals => sare la else

  Executie pas cu pas:

  Dupa FPADDR.i -2:
    [7]  &FP_fact[-2].i  (pointer catre n)    <-- SP
         ^^^ aceasta e ADRESA lui n, nu valoarea lui n

  Dupa LOAD.i:
    [7]  3 (int)                               <-- SP
         ^^^ pop adresa, push valoarea de la acea adresa

  Dupa PUSH.i 3:
    [7]  3
    [8]  3                                     <-- SP

  Dupa LESS.i:
    [7]  0 (int, rezultat 3<3=false)           <-- SP
         pop 3 (top=3), pop 3 (before=3), push (3 < 3) = 0

  JF [else_label]: valoarea 0 e falsa => salt la else
    Pop 0 de pe stiva. SP revine la [6] (= FP_fact)
    IP = else_label (= instructiunea "return n*fact(n-1)")

================================================================================
  PASUL 6 — Bransa else: return n * fact(n-1)
================================================================================

  Cod generat:
    // n (stanga multiplicarii)
    FPADDR.i -2         // &n
    LOAD.i              // n = 3 (rval pentru stanga MUL)

    // fact(n-1): calculam argumentul
    FPADDR.i -2         // &n
    LOAD.i              // n = 3
    PUSH.i 1            // 1
    SUB.i               // 3 - 1 = 2 (argumentul pentru apelul recursiv)
    CALL fact           // apel recursiv fact(2)

    MUL.i               // n * fact(n-1) = 3 * 2 = 6
    RET 1               // return 6; 1 = nr de parametri

  ----- Dupa FPADDR.i -2 + LOAD.i (pentru n stang) -----
    [6]  FP_main        FP_fact[0]  <-- FP_fact
    [7]  3 (int)                    <-- SP

  ----- Calculam argumentul fact(n-1) -----
    Dupa FPADDR.i -2:  stack[8] = &n
    Dupa LOAD.i:       stack[8] = 3
    Dupa PUSH.i 1:     stack[9] = 1
    Dupa SUB.i:        stack[8] = 2  (3-1=2, SP la [8])

  ----- CALL fact (recursiv, cu n=2) -----
    Stiva inainte de CALL:
      [7]  3             // n=3 (stanga MUL, va astepta pe stiva)
      [8]  2             // argumentul pentru apelul recursiv  <-- SP

    CALL face push ret_addr_2:
      [9]  ret_addr_2    <-- SP

    ============================================================
    APEL RECURSIV: fact(2)
    ============================================================
    ENTER 0:
      [10] FP_fact       FP_fact2[ 0]  <-- FP_fact2, SP la [10]
      FP_fact2[-2] = 2   (n=2)
      FP_fact2[-1] = ret_addr_2
      FP_fact2[ 0] = FP_fact

    Corpul fact(2): if(2 < 3)
      FPADDR.i -2  => &FP_fact2[-2].i
      LOAD.i       => 2
      PUSH.i 3     => 3
      LESS.i       => (2 < 3) = 1   => JF NU sare (1 = adevarat)

    Bransa then: return n
      FPADDR.i -2  => &n
      LOAD.i       => 2
      RET 1        => returneaza 2

    RET 1 pentru fact(2):
      v = popv()          => salveaza Val{i=2}
      IP = FP_fact2[-1].p => IP = ret_addr_2 (instructiunea MUL.i din fact(3))
      SP = FP_fact2 - 1 - 2  = [10] - 3 = [7]
           ^^^ elimina: old_FP[10] + ret_addr_2[9] + argumentul 2[8]
           SP ajunge la [7] (sub argumentul 2, deasupra lui 3-stang)
      FP = FP_fact2[0].p  => FP = FP_fact  (restauram FP_fact)
      pushv(v)             => stack[8] = 2  SP = [8]
    ============================================================

  ----- Dupa RET din fact(2) -----
    Stiva:
      [7]  3    // n=3, operandul stang al MUL (a asteptat pe stiva)
      [8]  2    // valoarea returnata de fact(2)         <-- SP

  ----- MUL.i -----
    pop 2 (top), pop 3 (before)
    push (3 * 2) = 6
    Stiva:
      [7]  6                                             <-- SP

  ----- RET 1 (returneaza 6 din fact(3)) -----
    v = popv()           => Val{i=6}
    IP = FP_fact[-1].p   => IP = ret_addr_1 (CALL_EXT put_i din main)
    SP = FP_fact - 1 - 2 = [6] - 3 = [3]
         ^^^ elimina: old_FP[6] + ret_addr_1[5] + argumentul 3[4]
         SP la [3] (= FP_main[2] = i, sub frame-ul lui fact)
    FP = FP_fact[0].p    => FP = FP_main
    pushv(v)             => stack[4] = 6, SP = [4]

================================================================================
  PASUL 7 — Dupa RET din fact(3): stiva in main
================================================================================

    index   continut            relativ la FP_main
    ------  ------------------  -------------------
    [0]     &HALT               FP_main[-1]
    [1]     old_FP_0            FP_main[ 0]  <-- FP_main
    [2]     r (?)               FP_main[ 1]
    [3]     i (?)               FP_main[ 2]
    [4]     6 (int)                          <-- SP  (valoarea returnata de fact(3))

  Urmatoarea instructiune: CALL_EXT put_i
    put_i() face popi()  =>  pop 6, afiseaza "=> 6"
    SP revine la [3]

================================================================================
  PASUL 8 — r=1, i=2 (atribuiri)
================================================================================

  Cod generat pentru "r=1":
    FPADDR.i 1         // push &FP_main[1].i = adresa lui r
    PUSH.i 1           // push 1
    STORE.i            // pop 1 (val), pop &r (addr), *addr=1, push 1 inapoi
    DROP               // pop 1 (curata stiva)

  Cum arata FPADDR.i 1:
    Instructiunea pune pe stiva adresa membrului .i al celulei FP_main[1]
    Aceasta adresa = &(stack[2].i) (= adresa interna a valorii int din celula)

  STORE.i face (codul profesorului):
    iTop = popi()      => 1
    vAddr = popv()     => {.p = &stack[2].i}
    *(int*)vAddr.p = 1 => stack[2].i = 1   => r = 1
    pushi(iTop)        => pune inapoi 1 pe stiva
    // Valoarea 1 RAMANE pe stiva!

  DROP:
    popv()             => curata 1 de pe stiva

  Stiva dupa r=1:
    [2]  r = 1         FP_main[1]  (valoarea scrisa in celula)
    [3]  i = ?         FP_main[2]  <-- SP

  Acelasi mecanism pentru "i=2":
    FPADDR.i 2 / PUSH.i 2 / STORE.i / DROP
    => stack[3].i = 2   => i = 2

================================================================================
  PASUL 9 — while(i < 5){ r = r*i; i = i+1; }
================================================================================

  Cod generat:

  [while_cond]:   <-- tinta JMP (= beforeWhileCond->next)
    FPADDR.i 2         // &i
    LOAD.i             // i (rval)
    PUSH.i 5
    LESS.i             // i < 5 ?
    JF [while_end]

    // r = r*i
    FPADDR.i 1         // &r (destinatie, lval)
    LOAD.i             // r (rval, pentru calcul)
    FPADDR.i 2         // &i  -- ATENTIE: ordinea e gresita pentru STORE!
    ...

  ATENTIE la exprAssign pentru "r = r*i":
    Codul generat in exprAssign:
      1. Se emite codul pentru lval (r):
         FPADDR.i 1    // adresa lui r, ramane pe stiva ca lval
      2. Se emite codul pentru rval (r*i):
         FPADDR.i 1 / LOAD.i    // r ca rval
         FPADDR.i 2 / LOAD.i    // i ca rval
         MUL.i                  // r*i
      3. STORE.i:
         pop (r*i)
         pop &r
         *&r = r*i
         push r*i  (ramane pe stiva)
      4. DROP (din stm, deoarece tip != VOID)

  Iteratia 1 (i=2): r = 1*2 = 2, i = 2+1 = 3
  Iteratia 2 (i=3): r = 2*3 = 6, i = 3+1 = 4
  Iteratia 3 (i=4): r = 6*4 = 24, i = 4+1 = 5
  Iteratia 4 (i=5): 5 < 5 = 0 => JF => iesim din while

  [while_end]:
    NOP

================================================================================
  PASUL 10 — put_i(r)  si  RET_VOID
================================================================================

  Cod generat:
    FPADDR.i 1         // &r
    LOAD.i             // r = 24 (rval)
    CALL_EXT put_i     // afiseaza "=> 24"
    RET_VOID 0         // 0 parametri (main nu are param)

  RET_VOID 0:
    IP = FP_main[-1].p   => &HALT
    SP = FP_main - 0 - 2 = FP_main - 2 = stack[-1]  (stiva goala)
    FP = FP_main[0].p    => NULL (old_FP initial)

  Urmatoarea instructiune: HALT => run() returneaza

================================================================================
  REZUMAT: CE REPREZINTA FIECARE VALOARE DIN STIVA
================================================================================

  Exemplu: stiva la inceputul executiei corpului fact(3)

    index   valoare             ce reprezinta
    ------  ------------------  ------------------------------------------
    [0]     &HALT               adresa instructiunii HALT din entryCode
                                (= adresa de intoarcere a lui main catre entryCode)
    [1]     NULL                old_FP al main (FP era NULL inainte de main)
    [2]     r (?)               variabila locala r a lui main (FP_main[1])
    [3]     i (?)               variabila locala i a lui main (FP_main[2])
    [4]     3 (int)             argumentul n=3 al lui fact (FP_fact[-2])
    [5]     ret_addr_1          adresa de intoarcere din fact catre main
                                (= adresa instructiunii CALL_EXT put_i din main)
    [6]     FP_main             old_FP salvat al lui fact (FP_fact[0])
                                ^-- FP_fact pointeaza AICI

  Adresa de intoarcere (FP_fact[-1] = ret_addr_1):
    - Este adresa instructiunii din main care urmeaza dupa CALL fact
    - In exemplul nostru: instructiunea CALL_EXT put_i
    - Dupa RET, IP este setat la aceasta adresa => executia continua in main

  Valoarea 6:
    - Este rezultatul calculului 3 * fact(2) = 3 * 2 = 6
    - Dupa RET 1 din fact(3), e pusa pe stiva in locul parametrilor si frame-ului
    - Urmatoarea instructiune (CALL_EXT put_i) o consuma si afiseaza "=> 6"

================================================================================
  FORMULA ADRESELOR RELATIVE LA FP
================================================================================

  Pentru o functie cu nrParams parametri si nrLocali variabile locale:

  Parametrul k (0-indexed):
    adresa fata de FP = k - nrParams - 1
    Exemplu: n e primul param (k=0) din fact (nrParams=1):
             adresa = 0 - 1 - 1 = -2  =>  FP_fact[-2]

  Variabila locala k (0-indexed):
    adresa fata de FP = k + 1
    Exemplu: r e prima locala (k=0) din main:
             adresa = 0 + 1 = 1  =>  FP_main[1]
    Exemplu: i e a doua locala (k=1) din main:
             adresa = 1 + 1 = 2  =>  FP_main[2]

  De ce aceasta formula:
    Dupa CALL: stiva are [..., param_0, ..., param_{n-1}, ret_addr]
    Dupa ENTER: stiva are [..., param_0, ..., param_{n-1}, ret_addr, old_FP, loc_0, ..., loc_{m-1}]
                                                                      ^-- FP pointeaza AICI
    Deci:
      FP[-nrParams-1] = param_0  (cel mai vechi parametru)
      ...
      FP[-2]          = param_{n-1}  (ultimul parametru)
      FP[-1]          = ret_addr
      FP[ 0]          = old_FP   <-- FP
      FP[ 1]          = loc_0
      ...
      FP[nrLocali]    = loc_{m-1}

================================================================================
  NOTE IMPORTANTE DESPRE COMPORTAMENTUL STORE
================================================================================

  Profesorul implementeaza STORE_I astfel:
    iTop  = popi()          // extrage valoarea int
    vAddr = popv()          // extrage adresa (ca void*)
    *(int*)vAddr.p = iTop   // scrie la adresa
    pushi(iTop)             // pune INAPOI valoarea pe stiva

  Aceasta inseamna ca expresia de atribuire "x = 5" LASA valoarea 5 pe stiva.
  Acest lucru permite expresii de forma "a = b = 5" (atribuire inlantuita).
  Dar cand atribuirea e folosita ca STATEMENT (instructiune de sine statatoare),
  trebuie emis DROP dupa STORE pentru a curata stiva.

  Codul din stm() pentru expresie-statement:
    if(rExpr.type.tb != TB_VOID)
        addInstr(&owner->fn.instr, OP_DROP);

  Tipul rezultat al lui exprAssign e tipul destinatiei (non-void),
  deci DROP este intotdeauna emis dupa o atribuire ca statement.

================================================================================
