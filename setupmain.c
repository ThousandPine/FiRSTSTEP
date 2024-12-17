// .data
char G1 = '1';
// .bss
char G2;

// .text
void setupmain(void)
{
    char *vmem = (char *)0xB8000;
    *(vmem++) = 'P';
    *(vmem++) = 0x84;
    *(vmem++) = G1;
    *(vmem++) = 0x84;
    G2 = '2';
    *(vmem++) = G2;
    *(vmem++) = 0x84;

    while(1);
}