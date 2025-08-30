#include <avr/wdt.h>
#include "DeviceDriverSet_xxx0.h"
#include "ApplicationFunctionSet_xxx0.cpp"

DeviceDriverSet_Motor AppMotor;
DeviceDriverSet_ULTRASONIC myUltrasonic;
DeviceDriverSet_Servo Servos;
Application_xxx Carrito;

const int rapidin = 100;
const int recor = 60;   //cm por recorrido antes de detenerse a hacer mediciones.
const int delxcm = 30;  //delay supuesto por cada cm avanzado.
const int lentin = 50;
const int distpard = 40;  //Distancia de la pared a la que se desea detener el vehiculo cuando va de frente.
const int delxgrad = 10;  //tiempo por grado de giro.
const int toldesv = 3;    //Tolerancia de desviacion.
bool dg = false;          // Giro definido
bool pc = false;          // Fase de corrección
bool girando = false;
int antdis = 0;
int dis = 0;
bool pc_read = false;
int cont = 0;
ConquerorCarMotionControl giro = Forward;      //hacia donde está girando
ConquerorCarMotionControl antigiro = Forward;  //hacia donde no está girando.

int x = 0;

void setup() {
  Carrito.CMD_Miliscarro = millis();
  AppMotor.DeviceDriverSet_Motor_Init();
  myUltrasonic.DeviceDriverSet_ULTRASONIC_Init();
  Servos.DeviceDriverSet_Servo_Init(90);
  Serial.begin(9600);
}

void giro_esquina() {
  Serial.println("Gira en esquina");
  Movimiento_Robot(giro, 90);
  delay(750);
  Movimiento_Robot(stop_it, 0);
  delay(100);
  pc_read = false;
  antdis = 0;
  cont++;
}

void verificar() {
  int vr = 0, vl = 0;

  Movimiento_Robot(Carrito.Motion_Control = 8, 0);
  Servos.DeviceDriverSet_Servo_control(0);
  delay(700);
  myUltrasonic.DeviceDriverSet_ULTRASONIC_Get(&vr);
  Servos.DeviceDriverSet_Servo_control(180);
  delay(700);
  myUltrasonic.DeviceDriverSet_ULTRASONIC_Get(&vl);

  dg = true;
  pc = true;

  if (vr > vl) {
    giro = Right;
    antigiro = Left;
    //Servos.DeviceDriverSet_Servo_control(0);
  } else {
    giro = Left;
    antigiro = Right;
    //Servos.DeviceDriverSet_Servo_control(180);
  }
  giro_esquina();
}

void mediciones() {
  Movimiento_Robot(stop_it, 0);
  if (giro == Right)
    Servos.DeviceDriverSet_Servo_control(180);
  else
    Servos.DeviceDriverSet_Servo_control(0);
  antdis = dis;
  delay(400);
  myUltrasonic.DeviceDriverSet_ULTRASONIC_Get(&dis);
  if (antdis == 0)
    antdis = dis;
  Servos.DeviceDriverSet_Servo_control(90);
  delay(400);
  myUltrasonic.DeviceDriverSet_ULTRASONIC_Get(&x);
}

void recorre() {
  Movimiento_Robot(Forward, rapidin);
  if (x > distpard) {
    if (x - distpard > recor)
      delay(recor * delxcm);
    else
      delay((x - distpard) * delxcm);
  } else {
    giro_esquina();
  }
}

void corrige() {
  float dif = (float)(dis - antdis);
  float angulo_rad = atan(dif / recor);
  float angulo_deg = angulo_rad * (180.0 / PI);
  int tiempo_giro = (int)(fabs(angulo_deg));
  if (antdis > dis) {
    Movimiento_Robot(giro, 90);
    delay(delxgrad * tiempo_giro);
  } else {
    Movimiento_Robot(antigiro, 90);
    delay(delxgrad * tiempo_giro);
  }
}

void loop() {
  if (cont > 12) {
    return;
  }
  // **FASE INICIAL: Definir giro**
  if (!dg) {
    myUltrasonic.DeviceDriverSet_ULTRASONIC_Get(&x);
    if (x < distpard && x != 0) {
      verificar();
    } else {
      Movimiento_Robot(Forward, rapidin);
      delay(500);
    }
    return;
  }

  // **FASE DE CORRECCIÓN**
  if (pc) {
    if (!pc_read) {
      // Leer la distancia inicial de referencia
      mediciones();
      recorre();
      pc_read = true;
    } else {
      mediciones();
      if (toldesv < abs(dis - antdis))
        corrige();
      recorre();
    }
  }
}
