#include <msp430f5529.h>
//#include <stdio.h>
#include "fixed-point.h"

#define true 1
#define false 0

#define NUM_BYTES_TX 2
#define NUM_BYTES_RX 6
#define ADXL_345     0x53

int RXByteCtr;       // enables repeated start when 1
volatile unsigned char RxBuffer[6];         // Allocate 6 byte of RAM
unsigned char *PRxData;                     // Pointer to RX data
unsigned char TXByteCtr, RX = 0;
unsigned char MSData[2];
unsigned int wdtCounter = 0;

fp_t acc[3];
fp_t dir_filter_ref[3];

//////////////////// gesture 1
const fp_t quantizerMap[14][3] = {
  { 12990 , -321 , -7587 }, //0
  { 7048 , 0 , 7048 },
  { 0 , 0 , 9967 },
  { -7048 , 0 , 7048 },
  { -9967 , 0 , 0 },
  { -7048 , 0 , -7048 },
  { 1124 , -264 , -18219 }, //6
  { 7991 , -34 , -14040 }, //7
  { 0 , 9967 , 0 },
  { 0 , 7048 , 7048 },
  { 0 , -7048 , 7048 },
  { 0 , -9967 , 0 },
  { 0 , -7048 , -7048 },
  { 371 , 7838 , -13185 }}; //d

const fp_t b[112] = {
  1533, 5401, 5209, 3951, 1958, 0, 0, 0, // 0
  0, 10922, 10922, 10922, 0, 0, 0, 0,
  0, 0, 10922, 10922, 10922, 0, 0, 0,
  0, 0, 0, 10922, 10922, 10922, 0, 0,
  0, 0, 0, 0, 10922, 10922, 10922, 0,
  0, 0, 0, 0, 0, 10922, 10922, 10922,
  23533, 8183, 10277, 14808, 15356, 16325, 17108, 17255, // 6
  7157, 15432, 11460, 3073, 2611, 1647, 615, 0, // 7
  4, 47, 96, 243, 362, 518, 578, 509,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  538, 3703, 5724, 10690, 12477, 14276, 14466, 15003}; // d

const fp_t a[64] = {
  10922, 10922, 10922, 0, 0, 0, 0, 0,
  0, 10922, 10922, 10922, 0, 0, 0, 0,
  0, 0, 10922, 10922, 10922, 0, 0, 0,
  0, 0, 0, 10922, 10922, 10922, 0, 0,
  0, 0, 0, 0, 10922, 10922, 10922, 0,
  0, 0, 0, 0, 0, 10922, 10922, 10922,
  0, 0, 0, 0, 0, 0, 16384, 16384,
  0, 0, 0, 0, 0, 0, 0, 32767};

fp_t f[8];
fp_t s[8];
const fp_t pi[8] = {32767,0,0,0,0,0,0,0};


//////////////////// gesture 2
const fp_t quantizerMap2[14][3] = {
		  { 0 , -9967 , 0 },
		  { 0 , -7048 , -7048 },
		  { 371 , 7838 , -13185 },
  { 12990 , -321 , -7587 }, //0
  { 7048 , 0 , 7048 },
  { 0 , 0 , 9967 },
  { -7048 , 0 , 7048 },
  { -9967 , 0 , 0 },
  { -7048 , 0 , -7048 },
  { 1124 , -264 , -18219 }, //6
  { 7991 , -34 , -14040 }, //7
  { 0 , 9967 , 0 },
  { 0 , 7048 , 7048 },
  { 0 , -7048 , 7048 }};

const fp_t b2[112] = {
		  0, 0, 0, 0, 0, 0, 0, 0,
		  538, 3703, 5724, 10690, 12477, 14276, 14466, 15003,
  1533, 5401, 5209, 3951, 1958, 0, 0, 0, // 0
  0, 10922, 10922, 10922, 0, 0, 0, 0,
  0, 0, 10922, 10922, 10922, 0, 0, 0,
  0, 0, 0, 10922, 10922, 10922, 0, 0,
  0, 0, 0, 0, 10922, 10922, 10922, 0,
  0, 0, 0, 0, 0, 10922, 10922, 10922,
  23533, 8183, 10277, 14808, 15356, 16325, 17108, 17255, // 6
  7157, 15432, 11460, 3073, 2611, 1647, 615, 0, // 7
  4, 47, 96, 243, 362, 518, 578, 509,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0};

const fp_t a2[64] = {
  0, 0, 0, 0, 0, 0, 0, 32767,
  10922, 10922, 10922, 0, 0, 0, 0, 0,
  0, 10922, 10922, 10922, 0, 0, 0, 0,
  0, 0, 10922, 10922, 10922, 0, 0, 0,
  0, 0, 0, 10922, 10922, 10922, 0, 0,
  0, 0, 0, 0, 10922, 10922, 10922, 0,
  0, 0, 0, 0, 0, 10922, 10922, 10922,
  0, 0, 0, 0, 0, 0, 16384, 16384};

fp_t f2[8];
fp_t s2[8];
const fp_t pi2[8] = {32767,0,0,0,0,0,0,0};

char started = false;

void Setup_TX(unsigned char);
void Setup_RX(unsigned char);
void Transmit(unsigned char,unsigned char);
void TransmitOne(unsigned char);
void Receive(void);
void hmm();

int main(void)
{
	WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT

  // LED
  P1DIR |= BIT0;
  P1OUT |= BIT0;

  //printf("start\n");
  int i;
  while(true) {
    for(i=0;i<200;i++) { // ~ 12 secs
      acc[0] = ((((int)RxBuffer[1]) << 8) | RxBuffer[0]) << 6;
      acc[1] = ((((int)RxBuffer[3]) << 8) | RxBuffer[2]) << 6;
      acc[2] = ((((int)RxBuffer[5]) << 8) | RxBuffer[4]) << 6;
      hmm();
    }
    P1OUT ^= BIT0;
    //printf("done\n");
  }
}

//-------------------------------------------------------------------------------
// The USCI_B0 data ISR is used to move received data from the I2C slave
// to the MSP430 memory. It is structured such that it can be used to receive
// any 2+ number of bytes by pre-loading RXByteCtr with the byte count.
//-------------------------------------------------------------------------------
#pragma vector = USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void)
{
  if(RX == 1){                              // Master Recieve?
    RXByteCtr--;                              // Decrement RX byte counter
    if (RXByteCtr)
      {
        *PRxData++ = UCB0RXBUF;                 // Move RX data to address PRxData
      }
    else
      {
        UCB0CTL1 |= UCTXSTP;                // No Repeated Start: stop condition
        *PRxData++ = UCB0RXBUF;                   // Move final RX data to PRxData
        __bic_SR_register_on_exit(CPUOFF);      // Exit LPM0
      }}
  else{                                     // Master Transmit
    if (TXByteCtr)                        // Check TX byte counter
      {
        TXByteCtr--;                            // Decrement TX byte counter
        UCB0TXBUF = MSData[TXByteCtr];          // Load TX buffer
      }
    else
      {
          UCB0CTL1 |= UCTXSTP;                    // I2C stop condition
          UCB0IFG &= ~UCTXIFG;                     // Clear USCI_B0 TX int flag
          __bic_SR_register_on_exit(CPUOFF);      // Exit LPM0
      }
  }
}

void Setup_TX(unsigned char Dev_ID){
  _DINT();
  RX = 0;
  UCA0IE &= ~UCRXIE; // Disable USCI_A0 RX interrupt
  UCB0IE &= ~UCRXIE;
  while (UCB0CTL1 & UCTXSTP);               // Ensure stop condition got sent// Disable RX interrupt
  UCB0CTL1 |= UCSWRST;                      // Enable SW reset
  UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;     // I2C Master, synchronous mode
  UCB0CTL1 = UCSSEL_2 + UCSWRST;            // Use SMCLK, keep SW reset
  UCB0BR0 = 12;                             // fSCL = SMCLK/12 = ~100kHz
  UCB0BR1 = 0;
  UCB0I2CSA = Dev_ID;                       // Slave Address is 048h
  UCB0CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
  UCB0IE |= UCTXIE;                          // Enable TX interrupt
}

void Setup_RX(unsigned char Dev_ID){
  _DINT();
  RX = 1;
  UCA0IE &= ~UCRXIE; // Disable USCI_A0 RX interrupt
  UCB0IE &= ~UCTXIE;
  UCB0CTL1 |= UCSWRST;                      // Enable SW reset
  UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;     // I2C Master, synchronous mode
  UCB0CTL1 = UCSSEL_2 + UCSWRST;            // Use SMCLK, keep SW reset
  UCB0BR0 = 12;                             // fSCL = SMCLK/12 = ~100kHz
  UCB0BR1 = 0;
  UCB0I2CSA = Dev_ID;                       // Slave Address is 048h
  UCB0CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
  UCB0IE |= UCRXIE;                          // Enable RX interrupt
}

void Transmit(unsigned char Reg_ADD,unsigned char Reg_DAT){
  MSData[1]= Reg_ADD;
  MSData[0]= Reg_DAT;
  TXByteCtr = NUM_BYTES_TX;                  // Load TX byte counter
  while (UCB0CTL1 & UCTXSTP);             // Ensure stop condition got sent
  UCB0CTL1 |= UCTR + UCTXSTT;             // I2C TX, start condition
  __bis_SR_register(CPUOFF + GIE);        // Enter LPM0 w/ interrupts
}

void TransmitOne(unsigned char Reg_ADD){
  MSData[0]= Reg_ADD;
  TXByteCtr = 1;                  // Load TX byte counter
  while (UCB0CTL1 & UCTXSTP);             // Ensure stop condition got sent
  UCB0CTL1 |= UCTR + UCTXSTT;             // I2C TX, start condition
  __bis_SR_register(CPUOFF + GIE);        // Enter LPM0 w/ interrupts
}

void Receive(void){
  PRxData = (unsigned char *)RxBuffer;    // Start of RX buffer
  RXByteCtr = NUM_BYTES_RX;             // Load RX byte counter
  while (UCB0CTL1 & UCTXSTP);             // Ensure stop condition got sent
  UCB0CTL1 |= UCTXSTT;                    // I2C start condition
  __bis_SR_register(CPUOFF + GIE);        // Enter LPM0 w/ interrupts
}


//-------------------------------------------------------------------------------
// HMM
//-------------------------------------------------------------------------------

char filter(){
  fp_t abs = fp_add(fp_add(fp_mul(acc[0], acc[0]),
                           fp_mul(acc[1], acc[1])),
                    fp_mul(acc[2], acc[2]));
  ////////////////////////////////////////////////////////////////////////////////
  //idle state filter
  if (!(fp_cmp(abs, 10486)==1 /*0.32*/ ||
        fp_cmp(abs, 8847)==2 /*0.27*/)) { // if between 0.01 - 0.09
    return true;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // def = directional equivalence filter
  const fp_t def_sensitivity = 2621; //0.08
  if (fp_cmp(acc[0], fp_sub(dir_filter_ref[0], def_sensitivity))==2 ||
      fp_cmp(acc[0], fp_add(dir_filter_ref[0], def_sensitivity))== 1 ||
      fp_cmp(acc[1], fp_sub(dir_filter_ref[1], def_sensitivity))==2 ||
      fp_cmp(acc[1], fp_add(dir_filter_ref[1], def_sensitivity))== 1 ||
      fp_cmp(acc[2], fp_sub(dir_filter_ref[2], def_sensitivity))==2 ||
      fp_cmp(acc[2], fp_add(dir_filter_ref[2], def_sensitivity))==1) {
    dir_filter_ref[0] = acc[0];
    dir_filter_ref[1] = acc[1];
    dir_filter_ref[2] = acc[2];
    return true;
  }
  return true;
}

//The quantizer, maps accelerometer readings to a set of integers.
unsigned char derive_group(){
  fp_t a, b, c, d;
  fp_t minDist = 0x7fff; //0x7fff;
  char minGroup=0;
  fp_t *ref;
  char i;

  for (i = 0; i < 14; i++){
    ref = quantizerMap[i];
    a = fp_sub(ref[0], acc[0]);
    b = fp_sub(ref[1], acc[1]);
    c = fp_sub(ref[2], acc[2]);
    d = fp_add(fp_add(fp_mul(a,a), fp_mul(b,b)), fp_mul(c,c));
    if (fp_cmp(d, minDist) == 2){
      minDist = d;
      minGroup = i;
    }
  }
  /* UARTSendArray("group=", 6); */
  /* UARTSendInt(minGroup); */
  return minGroup;
}

unsigned char derive_group2(){
  fp_t a, b, c, d;
  fp_t minDist = 0x7fff; //0x7fff;
  char minGroup=0;
  fp_t *ref;
  char i;

  for (i = 0; i < 14; i++){
    ref = quantizerMap2[i];
    a = fp_sub(ref[0], acc[0]);
    b = fp_sub(ref[1], acc[1]);
    c = fp_sub(ref[2], acc[2]);
    d = fp_add(fp_add(fp_mul(a,a), fp_mul(b,b)), fp_mul(c,c));
    if (fp_cmp(d, minDist) == 2){
      minDist = d;
      minGroup = i;
    }
  }
  /* UARTSendArray("group=", 6); */
  /* UARTSendInt(minGroup); */
  return minGroup;
}

//Performs the next iteration of the HMM forward algorithm
fp_t forward_proc_inc(unsigned char o){
  fp_t ord = 0;
  fp_t sum;
  unsigned char k,l;

  if (started == false){
    for (l = 0; l < 8; l++){
      f[l] = fp_mul(pi[l],b[(o<<3) + l]); // pi*b
      //f[l] = b[o][l];
    }
    return 0;
  }else{
    for (k = 0; k < 8; k++){
      sum = 0;
      for (l = 0; l < 8; l++){
        sum = fp_add(sum, fp_mul(s[l], a[(l<<3) + k]));
        //sum = fp_add(sum, fp_mul(s[l], b[(l<<3) +k]));
      }
      f[k] = fp_mul(sum, b[(o<<3)+k]);
      //f[k] = fp_mul(sum, b[o][k]);
      ord |= f[k];
    }
  }
  return ord;
}


fp_t forward_proc_inc2(unsigned char o){
  fp_t ord = 0;
  fp_t sum;
  unsigned char k,l;

  if (started == false){
    for (l = 0; l < 8; l++){
      f2[l] = fp_mul(pi2[l],b2[(o<<3) + l]); // pi*b
      //f[l] = b[o][l];
    }
    return 0;
  }else{
    for (k = 0; k < 8; k++){
      sum = 0;
      for (l = 0; l < 8; l++){
        sum = fp_add(sum, fp_mul(s2[l], a2[(l<<3) + k]));
        //sum = fp_add(sum, fp_mul(s2[l], b2[(l<<3) +k]));
      }
      f2[k] = fp_mul(sum, b2[(o<<3)+k]);
      //f[k] = fp_mul(sum, b2[o][k]);
      ord |= f2[k];
    }
  }
  return ord;
}


//Called at the end of the gesture,
//Returns the id of the recognized gesture or -1 if none.
char input_end(){
  fp_t prob, prob2;
  char recognized; // which gesture has been recognized
  char j;

  started = false;
    prob = 0;
    prob2 = 0;
    // add probabilities
    for (j = 0; j < 8; j++){
      prob = fp_add(prob, s[j]);
      prob2 = fp_add(prob2, s2[j]);
    }
    if (fp_cmp(prob, prob2)==1) {
      recognized = 0;
    } else {
    	recognized = 1;
    }
  //printf("\np = %d %d\n", prob, prob2);

  return recognized;
}

//int pass = 0;

//Called with each accelerometer reading
void input_reading(){
  fp_t ord = 0;
  char l;
  if (filter()){
	//printf("%d %d %d\n", acc[0], acc[1], acc[2]);
    //pass++;
      ord = forward_proc_inc(derive_group()) | forward_proc_inc2(derive_group2());

    //counts the number of bits we can shift left by - the leading zeros
    char n = 0;
    while(ord > 0){
      n += 1;
      ord = ord << 1;
    }
    if (n>4) {
      n -= 4;
    }
    else {
      n = 0;
    }
    for (l = 0; l < 8; l++) {
      s[l] = f[l] << n;
      s2[l] = f2[l] << n;
    }
    if (started == false) started = true;
    // constantly moving ~35 s for 1000 iter   => 29 iter/s => 34 ms
    // one gestuer => 5 s
  } else {
    //__delay_cycles(1000);  // delay 1 ms
    // 3.9 s
  }
}

int iter = 0;
void hmm() {
  iter++;
  input_reading();
  if(iter == 1000) {
    char out = input_end();
    //printf("out=%d\n",out);
    //UARTSendArray("out=", 4);
    //UARTSendInt(out);
    /* UARTSendArray("pass=", 5); */
    /* UARTSendInt(pass); */
    /* pass = 0; */
    iter = 0;
    //__delay_cycles(1000);  // delay 1 ms
  }
}
