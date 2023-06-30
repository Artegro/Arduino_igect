// version 0.3.2
// manifold absolute pressure
// стабильный хх
unsigned long injtiming; // базовая длительность впрыска
unsigned long injectortiming; // скорректированная длительность впрыска
int TPS; //флаг заслонка закрыта
int IDL; //флаг холостой ход
int RPM; //скорость вращения двигателя
unsigned long lastflash; // время предыдущего оборота

const int TempPin = A1;  // вход температура двигателя
const int throttlePin = A2; //вход положения дз 
const int mapPin = A3; //вход сигнала дад 
int sensorTEMP;   //температура
int mapsensor; //показания map
int throttleposition;  //положение дроссельной заслонки
float tpsKOEF; // коэффициент коррекции по дросселю
float tempKOEF; // коэффициент коррекции по температуре
float rpmKOEF; //коэффициент коррекции по оборотам

void setup()
{
  Serial.begin(4800);
  // --> конфигурация выводов ардуино
  attachInterrupt(0, injector, RISING); //при повышении сигнала на D2 плюёмся бензином
  attachInterrupt(1, taho, RISING); //при повышении сигнала на D3 считаем обороты
    
  pinMode(2, INPUT_PULLUP); //вход дпкв
  pinMode(3, INPUT_PULLUP); //вход тахометра
  pinMode(TempPin, INPUT); // вход датчика температуры
  pinMode(throttlePin, INPUT); // вход дпдз
  pinMode(mapPin, INPUT); //вход map
  pinMode(13, OUTPUT); //отсюда подаем импульс на ключ форсунки
  pinMode(12, OUTPUT); //отсюда подаем сигнал на ключ реле бензонасоса
  // <-- конфигурация выводов ардуино
  
  // --> предпусковая подкачка топлива
  digitalWrite(12, HIGH); //запускаем бензонасос. делаем в сетапе, чтобы сработало один раз
  delay(2000); // включаем насос на 2 сек при запуске ардуины
  digitalWrite(12, LOW);  //выключаем бензонасос
  // <-- предпусковая подкачка топлива
}

void injector() //данная функция управляет форсункой
{
  digitalWrite(13, HIGH);            //включаем форсунку
  delayMicroseconds(injectortiming); //неправильно, i know, но я слишком тупой, чтобы сделать как надо, да и незачем - и так работает. можно начинать кидаться говном
  digitalWrite(13, LOW);             //выключаем форсунку

  //на самом деле, на рабочих 5500-6000 оборотах время одного оборота составляет 10-11 мс, (другие диапазоны нас мало интересуют, так как у нас вариатор)
  //при самых позитивных раскладах время впрыска, оно же время на которое застопорится мега при использовании delay
  //не превышает 4 мс. остается еще 6-7 мс на обработку кода. 
  //учитывая что при выходе на рабочий режим 5500-6000 каждые новые показания получаемые с датчиков вряд ли будут сильно отличаться от предыдущих - считаю допустима
  //некая инерционность, при этом точность и скорость обработки входящих данных не так сильно важна. в теории.
  // UPD: практика показала, что теория верна.
}

void taho() //данная функция считает обороты двигателя
{
  RPM = 60 / ((float)(millis() - lastflash) / 1000);  //считаем обороты
  lastflash = millis();                               //в данный момент времени
}


void loop() //в цикле происходит обработка датчиков и производится расчет длительности впрыска
{
  // обороты двигателя
  if ((millis() - lastflash) > 1000) 
  {                       
    RPM  = 0;   // если вращения нет больше 1 секунды, то rpm = 0, нужно для остановки насоса - нефиг качать на заглохшем двигателе
  }
  
  if (RPM >= 0 && RPM < 500) // диапазон оборотов 0-500 - прокрутка стартером
  {
    rpmKOEF = 1.2; // обогащаем для запуска
  }
  else
  {
    rpmKOEF = 1; // рабочий режим
  }
  
   // датчик абсолютного давления
  
  mapsensor = analogRead(mapPin); //посчитаем тут, обрабатывать будем потом
  mapsensor = map(mapsensor, 0, 1023, 0, 138); //транспонирование показаний дад

   // дад
  
   
  // температура двигателя --->
  sensorTEMP = analogRead(TempPin);
  sensorTEMP = map(sensorTEMP, 0, 1023, 100, 0); //
  if (sensorTEMP >= 40 && sensorTEMP < 50) //корректируем по температуре  
  {
    tempKOEF = 1.05; //обогащаем смесь, если прохладно
  }
  else
  {
    if (sensorTEMP >= 30 && sensorTEMP < 40)
    {
    tempKOEF = 1.1; // обогащаем если холодно
    }
    else
    {
      if (sensorTEMP < 30)
      {
        tempKOEF = 1.2; // добавляем еще если совсем холодно
      }
      else
      {
        tempKOEF = 1; //не трогаем, если прогреты
      }
    }
  }
  // температура двигателя <---

  // дроссель
  
  throttleposition = analogRead(throttlePin);
  throttleposition = map(throttleposition, 0, 1023, 100, 0); //транспонирование показаний тпс

  
  if (throttleposition >= 0 && throttleposition <= 7) // пусть пока будет х процентов положения заслонки - холостой ход, нужно для отсечки подачи топлива на сбросе газа
  {
    TPS = 1; // если хх, то выставляем флаг заслонка закрыта
  }
  else
  {
    TPS = 0; // если нет - флаг 0
  }
    
  // отсечка топливоподачи на сбросе газа 
  if (RPM > 3500 && TPS == 1) //если обороты больше RPM и дроссель закрыт
  {
    tpsKOEF = 0; //отключаем подачу топлива
  }
  else
  {
    tpsKOEF = 1+(throttleposition/1.3)*0.01; //коррекция смеси по положению дрочеля
  }
  
                                                                                 // для чего это вообще нужно? если при движении накатом с полностью закрытой дроссельной заслонкой
    //РАБОТАЕТ ТОПОРНО - РЫВКИ НА ПЕРЕХОДЕ, НАДО КАК-ТО СГЛАДИТЬ                 // в двигатель продолжает подаваться топливо, то при следующем открытии дросселя 
                                                                                 // двигатель никуда не поедет,  так как его просто зальёт 

  // если двигатель прокручивается, то включаем топливный насос
  if (RPM > 200)
  {
    digitalWrite(12, HIGH);
  }
  else
  {
    digitalWrite(12, LOW); //если не крутится то насос off
  }

    //рассчитываем базовую длительность впрыска исходя из показаний дад
   //линейная характеристика f(x) = kx+b, тогда х = (mapsensor + b)/k
  //при уменьшеннии k характеристика сдвигается в плюс по времени                   // в текстовом виде сумбурно - необходимо визуализировать для лучшего понимания
 //при увеличении b характеристика сдвигается в плюс по давлению
//при взаимном изменении k и b можно изменять крутизну характеристики 

  injtiming = (float)(mapsensor+30)/0.055; //сразу переводим в микросекунды
    /// МАХМУД, ПОДЖИГАЙ!
  if (TPS == 1 && RPM <= 3000) //если хх и обороты меньше 3000
  {
    injectortiming = (float)2100*tpsKOEF*tempKOEF*rpmKOEF; // исключаем из расчета показания дад, для стабилизации хх
    IDL = 1;
  }
  else
  {
    injectortiming = (float)injtiming*tpsKOEF*tempKOEF*rpmKOEF; // режим нагрузок
    IDL = 0;
  } 
  /*/ / отладка, а ля диагностика
  
  Serial.print("RPM: ");
  Serial.print(RPM);
  Serial.print("   TPS: ");
  Serial.print(throttleposition);
  Serial.print("   TINJ: ");
  Serial.print(injectortiming);
  Serial.print("   TMP:  ");
  Serial.print(sensorTEMP);
  Serial.print("   MAP:  ");
  Serial.print(mapsensor);
  Serial.print("   tpsK:  ");
  Serial.print(tpsKOEF);
  Serial.print("   tempK:  ");
  Serial.print(tempKOEF);
  Serial.print("   rpmK:  ");
  Serial.print(rpmKOEF);
  Serial.print("   IDLING:  ");
  Serial.print(IDL);
  Serial.print("   injBasic:  ");
  Serial.println(injtiming);
  
  // / отладка */
} //конец страницы



