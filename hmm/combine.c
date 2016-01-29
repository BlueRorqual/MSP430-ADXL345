//  Interfacing ADXL345 accelerometer with MSP430G2553 with I2C communication
//  and printing restuls to serial port using UART.
//
//                                /|\  /|\
//               ADXL345          10k  10k     MSP430G2xx3
//                slave            |    |        master
//             -----------------   |    |  -----------------
//            |              SDA|<-|---+->|P1.7/UCB0SDA  XIN|-
//            |                 |  |      |                 |
//            |                 |  |      |             XOUT|-
//            |              SCL|<-+----->|P1.6/UCB0SCL     |
//            |                 |         |                 |
//
//  For Sparkfun ADXL345,
//    * connect SDO to ground
//    * connect CS to VCC
//
//  Original Code Made By :
//  Prof. Ravi Butani
//  Marwadi Education Foundation, Rajkot GUJARAT-INDIA
//  ravi.butani@marwadieducation.edu.in
//  https://e2e.ti.com/support/microcontrollers/msp430/f/166/t/260094
//
//  Modified By :
//  Phitchaya Mangpo Phothilimthana
//  mangpo@eecs.berkeley.edu

//******************************************************************************
#include <msp430g2553.h>
#include "fixed-point.h"

#define true 1
#define false 0

#define NUM_BYTES_TX 2  
#define NUM_BYTES_RX 6
#define ADXL_345     0x53

int RXByteCtr, RPT_Flag = 0;       // enables repeated start when 1
volatile unsigned char RxBuffer[8];         // Allocate 6 byte of RAM
unsigned char *PRxData;                     // Pointer to RX data
unsigned char TXByteCtr, RX = 0;
unsigned char MSData[3];
unsigned int wdtCounter = 0;

fp_t acc[3];
fp_t dir_filter_ref[3];
fp_t quantizerMap[14][3];
fp_t b[112];
fp_t a[8][8];
fp_t f[8];
fp_t s[8];
fp_t last_bit = 1 << FP_BITS-1;
char started = false;

void Setup_TX(unsigned char);
void Setup_RX(unsigned char);
void Transmit(unsigned char,unsigned char);
void TransmitOne(unsigned char);
void Receive(void);
void Setup_UART();
void UARTSendArray(unsigned char *TxArray, unsigned char ArrayLength);
void UARTSendInt(unsigned int x);
void hmm();

int main(void)
{
  // LED
  P1DIR |= BIT0;
  P1OUT |= BIT0;

  // UART
  BCSCTL1 = CALBC1_1MHZ; // Set DCO to 1MHz
  DCOCTL = CALDCO_1MHZ; // Set DCO to 1MHz

  // Configure hardware UART
  P1SEL |= BIT1 + BIT2 ; // P1.1 = RXD, P1.2=TXD
  P1SEL2 |= BIT1 + BIT2 ; // P1.1 = RXD, P1.2=TXD
    
  Setup_UART();
  UARTSendArray("Hello\n", 6);
  __delay_cycles(1000);

  // ADXL345
  P1SEL  |= BIT6 + BIT7;                     // Assign I2C pins to USCI_B0
  P1SEL2 |= BIT6 + BIT7;                     // Assign I2C pins to USCI_B0

  // Init sequence for ADXL345
  //Transmit process
  Setup_TX(ADXL_345);
  RPT_Flag = 0;
  Transmit(0x2D,0x00);                    // STUCK
  while (UCB0CTL1 & UCTXSTP);             // Ensure stop condition got sent

  //Transmit process
  Setup_TX(ADXL_345);
  RPT_Flag = 0;
  Transmit(0x2D,0x10);
  while (UCB0CTL1 & UCTXSTP);             // Ensure stop condition got sent

  //Transmit process
  Setup_TX(ADXL_345);
  RPT_Flag = 0;
  Transmit(0x2D,0x08);
  while (UCB0CTL1 & UCTXSTP);             // Ensure stop condition got sent
  
  // Un-comment next block to change range of ADXL345
  /*
    Setup_TX(ADXL_345);
    RPT_Flag = 1;
    Transmit(0x31,0x01);                            // Range Select at add 0x31 write 0x00 for 2g(default)/ 0x01 for 4g/ 0x02 for 8g/ 0x03 for 16g
    while (UCB0CTL1 & UCTXSTP);         // Ensure stop condition got sent
  */

  /* Set watchdog timer interval to 1000ms (requires external crystal to work) */
  WDTCTL = WDT_ADLY_1000;

  /* "Interrupt enable 1" for the Watchdog Timer interrupt */
  IE1 |= WDTIE;

  /* Go into low power mode 3, general interrupts enabled */
  __bis_SR_register( LPM3_bits + GIE );

  /* Do nothing...forever */
  for( ; ; ) { }
}


/* Watchdog Timer interrupt service routine.  The function prototype
 *  tells the compiler that this will service the Watchdog Timer, and
 *  then the function follows.
 *    */
void watchdog_timer(void) __attribute__((interrupt(WDT_VECTOR)));
void watchdog_timer(void)
{
    wdtCounter++;
    /* Count 1 interrupts x 1000ms = 1000ms, or one second */
    if(wdtCounter == 1) {
      //Transmit process
      Setup_TX(ADXL_345);
      RPT_Flag = 0;
      TransmitOne(0x32);                      // Request Data from ADXL345
      while (UCB0CTL1 & UCTXSTP);             // Ensure stop condition got sent

      //Receive process
      Setup_RX(ADXL_345);
      Receive();
      while (UCB0CTL1 & UCTXSTP);             // Ensure stop condition got sent

      acc[0] = ((((int)RxBuffer[1]) << 8) | RxBuffer[0]) << 6;
      acc[1] = ((((int)RxBuffer[3]) << 8) | RxBuffer[2]) << 6;
      acc[2] = ((((int)RxBuffer[5]) << 8) | RxBuffer[4]) << 6;
    
      // now You have XYZ axis reading in x1,x2,x3 variable....Bingo... you can play with it as you like....
      // Below if sense x and y angle and Red led is on if its more then 45 or less then -45...
      // you can put your own condition here...

      /* if ((x1 > 128) || (y1 > 128) || (x1 < -128) || (y1 < -128)) { */
      /*   P1OUT |= BIT0; // red led on */
      /* } */
      /* else { */
      /*   P1OUT &= ~BIT0; // red led off */
      /* } */

      Setup_UART();
      UARTSendArray("sample\n", 7);
      UARTSendInt(acc[0]);
      UARTSendInt(acc[1]);
      UARTSendInt(acc[2]);
      hmm();
      /* Reset the counter for the next blink */
      wdtCounter = 0;
    }
}

//-------------------------------------------------------------------------------
// The USCI_B0 data ISR is used to move received data from the I2C slave
// to the MSP430 memory. It is structured such that it can be used to receive
// any 2+ number of bytes by pre-loading RXByteCtr with the byte count.
//-------------------------------------------------------------------------------
#pragma vector = USCIAB0TX_VECTOR
__interrupt void USCIAB0TX_ISR(void)
{
  
  if(RX == 1){                              // Master Recieve?
    RXByteCtr--;                              // Decrement RX byte counter
    if (RXByteCtr)
      {
        *PRxData++ = UCB0RXBUF;                 // Move RX data to address PRxData
      }
    else
      {
        if(RPT_Flag == 0)
          UCB0CTL1 |= UCTXSTP;                // No Repeated Start: stop condition
        if(RPT_Flag == 1){                    // if Repeated Start: do nothing
          RPT_Flag = 0;
        }
        *PRxData++ = UCB0RXBUF;                   // Move final RX data to PRxData
        __bic_SR_register_on_exit(CPUOFF);      // Exit LPM0
      }}
  else{                                     // Master Transmit
    if (TXByteCtr)                        // Check TX byte counter
      {
        UCB0TXBUF = MSData[TXByteCtr];          // Load TX buffer
        TXByteCtr--;                            // Decrement TX byte counter
      }
    else
      {
        /* UCB0CTL1 |= UCTXSTP;                    // I2C stop condition */
        /* IFG2 &= ~UCB0TXIFG;                     // Clear USCI_B0 TX int flag */
        if(RPT_Flag == 1){
          RPT_Flag = 0;
          TXByteCtr = NUM_BYTES_TX;                // Load TX byte counter
          __bic_SR_register_on_exit(CPUOFF);
        }
        else{
          UCB0CTL1 |= UCTXSTP;                    // I2C stop condition
          IFG2 &= ~UCB0TXIFG;                     // Clear USCI_B0 TX int flag
          __bic_SR_register_on_exit(CPUOFF);      // Exit LPM0
        }
      }
  }
}

void Setup_UART() {
  _DINT();
  IE2 &= ~UCB0RXIE; 
  IE2 &= ~UCB0TXIE; 
  UCA0CTL1 |= UCSSEL_2; // Use SMCLK
  UCA0BR0 = 104; // Set baud rate to 9600 with 1MHz clock (Data Sheet 15.3.13)
  UCA0BR1 = 0; // Set baud rate to 9600 with 1MHz clock
  UCA0MCTL = UCBRS0; // Modulation UCBRSx = 1
  UCA0CTL1 &= ~UCSWRST; // Initialize USCI state machine
  IE2 |= UCA0RXIE; // Enable USCI_A0 RX interrupt
}

void Setup_TX(unsigned char Dev_ID){
  _DINT();
  RX = 0;
  IE2 &= ~UCA0RXIE; // Disable USCI_A0 RX interrupt
  IE2 &= ~UCB0RXIE; 
  while (UCB0CTL1 & UCTXSTP);               // Ensure stop condition got sent// Disable RX interrupt
  UCB0CTL1 |= UCSWRST;                      // Enable SW reset
  UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;     // I2C Master, synchronous mode
  UCB0CTL1 = UCSSEL_2 + UCSWRST;            // Use SMCLK, keep SW reset
  UCB0BR0 = 12;                             // fSCL = SMCLK/12 = ~100kHz
  UCB0BR1 = 0;
  UCB0I2CSA = Dev_ID;                       // Slave Address is 048h
  UCB0CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
  IE2 |= UCB0TXIE;                          // Enable TX interrupt
}

void Setup_RX(unsigned char Dev_ID){
  _DINT();
  RX = 1;
  IE2 &= ~UCA0RXIE; // Disable USCI_A0 RX interrupt
  IE2 &= ~UCB0TXIE; 
  UCB0CTL1 |= UCSWRST;                      // Enable SW reset
  UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;     // I2C Master, synchronous mode
  UCB0CTL1 = UCSSEL_2 + UCSWRST;            // Use SMCLK, keep SW reset
  UCB0BR0 = 12;                             // fSCL = SMCLK/12 = ~100kHz
  UCB0BR1 = 0;
  UCB0I2CSA = Dev_ID;                       // Slave Address is 048h
  UCB0CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
  IE2 |= UCB0RXIE;                          // Enable RX interrupt
}

void Transmit(unsigned char Reg_ADD,unsigned char Reg_DAT){
  MSData[2]= Reg_ADD;
  MSData[1]= Reg_DAT;
  TXByteCtr = NUM_BYTES_TX;                  // Load TX byte counter
  while (UCB0CTL1 & UCTXSTP);             // Ensure stop condition got sent
  UCB0CTL1 |= UCTR + UCTXSTT;             // I2C TX, start condition
  __bis_SR_register(CPUOFF + GIE);        // Enter LPM0 w/ interrupts
}

void TransmitOne(unsigned char Reg_ADD){
  MSData[1]= Reg_ADD;
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

void UARTSendArray(unsigned char *TxArray, unsigned char ArrayLength){

  while(ArrayLength--){ // Loop until StringLength == 0 and post decrement
    while(!(IFG2 & UCA0TXIFG)); // Wait for TX buffer to be ready for new data
    UCA0TXBUF = *TxArray; //Write the character at the location specified py the pointer
    TxArray++; //Increment the TxString pointer to point to the next character
  }
  IFG2 &= ~UCA0TXIFG;                     // Clear USCI_A0 int flag
}

void UARTSendInt(unsigned int x){
  unsigned char buff[10];
  unsigned char data[10];
  unsigned char index = 0, i = 0;

  while(x > 0) {
    unsigned char val = x % 16;
    if(val < 10)
      buff[index] = 48+val;
    else
      buff[index] = 97+val-10;
    index++;
    x /= 16;
  }
  buff[index] = '\n';

  while(index > 0) {
    index--;
    data[i] = buff[index];
    i++;
  }

  if(i==0) {
    data[0] = '0';
    i++;
  }
  data[i] = '\n';
  UARTSendArray(data, i+1);
}

//-------------------------------------------------------------------------------
// HMM
//-------------------------------------------------------------------------------

char filter(){
  fp_t abs = fp_add(fp_add(fp_mul(acc[0], acc[0]),
                           fp_mul(acc[1], acc[1])),
                    fp_mul(acc[2], acc[2]));
  //printf("abs = %.30f\n", fp2d(abs));
  UARTSendArray("abs=", 4);
  UARTSendInt(abs);
  ////////////////////////////////////////////////////////////////////////////////
  //idle state filter
  if (!(fp_cmp(abs, d2fp(0.31))==1 ||
        fp_cmp(abs, d2fp(0.28))==-1)) { // if between 0.01 - 0.09
    return false;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // def = directional equivalence filter
  fp_t def_sensitivity = d2fp(0.04);
  if (fp_cmp(acc[0], fp_sub(dir_filter_ref[0], def_sensitivity))==-1 ||
      fp_cmp(acc[0], fp_add(dir_filter_ref[0], def_sensitivity))== 1 ||
      fp_cmp(acc[1], fp_sub(dir_filter_ref[1], def_sensitivity))==-1 ||
      fp_cmp(acc[1], fp_add(dir_filter_ref[1], def_sensitivity))== 1 ||
      fp_cmp(acc[2], fp_sub(dir_filter_ref[2], def_sensitivity))==-1 ||
      fp_cmp(acc[2], fp_add(dir_filter_ref[2], def_sensitivity))==1) {
    dir_filter_ref[0] = acc[0];
    dir_filter_ref[1] = acc[1];
    dir_filter_ref[2] = acc[2];
    return true;
  }
  return false;
}

//The quantizer, maps accelerometer readings to a set of integers.
char derive_group(){
  fp_t a, b, c, d;
  fp_t minDist = 0x7f; //0x7fff;
  char minGroup=0;
  fp_t *ref;
  char i;

  for (i = 0; i < 14; i++){
    ref = quantizerMap[i];
    a = fp_sub(ref[0], acc[0]);
    b = fp_sub(ref[1], acc[1]);
    c = fp_sub(ref[2], acc[2]);
    d = fp_add(fp_add(fp_mul(a,a), fp_mul(b,b)), fp_mul(c,c));
    if (fp_cmp(d, minDist) == -1){
      minDist = d;
      minGroup = i;
    }
  }
  UARTSendArray("group=", 6);
  UARTSendInt(minGroup);
  return minGroup;
}

//Performs the next iteration of the HMM forward algorithm
fp_t forward_proc_inc(char o){
  fp_t ord = 0;
  fp_t sum;
  char k,l;

  if (started == false){
    UARTSendArray("init\n", 5);
    for (l = 0; l < 8; l++){
      f[l] = b[(o<<3) + l]; // pi*b
    }
    for (l = 1; l < 8; l++){
      f[l] = 0;
    }
    started = true;
    return 0;
  }else{
    UARTSendArray("proc\n", 5);
    for (k = 0; k < 8; k++){
      sum = 0;
      for (l = 0; l < 8; l++){
        sum = fp_add(sum, fp_mul(s[l], a[l][k]));
      }
      f[k] = fp_mul(sum, b[(o<<3)+k]);
      ord |= f[k];
    }
  }
  return ord;
  }

//Called with each accelerometer reading
void input_reading(){
  fp_t ord = 0;
  char i,l;
  if (filter()){
    for (i = 0; i < 2; i++){
      char group = derive_group();
      ord |= forward_proc_inc(group);
    }

    //counts the number of bits we can shift left by - the leading zeros
    // : count 0
    // : count1 over if 2* over 1 . + count1 ; then drop ;
    char n = 0;
    while (ord && !(ord & last_bit)){
      n += 1;
      ord = ord << 1;
    }
    if (n>3){
      n-=3;
      for (l = 0; l < 8; l++){
        s[l] = f[l] << n;
        f[l] = s[l] << n;
      }
    }
  }
}

//Called at the end of the gesture,
//Returns the id of the recognized gesture or -1 if none.
char input_end(){
  fp_t prob;
  char recognized = -1; // which gesture has been recognized
  fp_t recogprob = -1; // probability of this gesture
  fp_t tmpgesture;
  char i, j;

  started = false;
  for (i = 0; i < 2; i++){
    prob = 0;
    // add probabilities
    for (j = 0; j < 8; j++){
      prob = fp_add(prob, s[j]);
    }
    if (fp_cmp(prob, recogprob)==1) {
      recogprob = prob;
      recognized = i;
    }
  }
  
  //printf("m->prob = %.30f\n", fp2d(recogprob));
  
  //dir_filter_ref[0] = 0; //reset for next time
  return recognized;
}

int iter = 0;
void hmm() {
  UARTSendArray("iter=", 5);
  UARTSendInt(iter);
  iter++;
  input_reading();
  if(iter == 20) {
    iter = 0;
    char out = input_end();
    UARTSendArray("out=", 4);
    UARTSendInt(out);
  }
  UARTSendArray("\n", 1);
}
