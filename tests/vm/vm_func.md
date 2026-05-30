f(2.0);
void f(double n){
    double i = 0.0;
    while(i < n){
        put_d(i);
        i = i + 0.5;
    }
}


n           ret_adr         old_FP          i
-2          -1              0               1


PUSH.f      2.0
CALL        f

                ENTER           1
                PUSH.f          0.0
                FPSTORE.f       1

L1:
                FPLOAD.f        1
                FPLOAD.f        -2
                LESS.f
                JF              L2

                FPLOAD.f        1
                CALL_EXT        put_d

                FPLOAD.f        1
                PUSH.f          0.5
                ADD.f
                FPSTORE.f       1

                JMP             L1

L2:
                RET_VOID        1

halt
