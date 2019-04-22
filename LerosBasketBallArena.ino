#include <LiquidCrystal_I2C.h>
#include <WString.h>

#define UP 0
#define DOWN 1

#define UP_BTN_PIN 2                //Pin of the Yellow push button highlighting in a menu the option above
#define DOWN_BTN_PIN 3              //Pin of the White push button highlight in a menu the option below
#define SETUP_BTN_PIN 4             //Pin of the Red push button to SETUP/SELECT/ABORT selection

#define RED_LIGHTS_BELOW_PIN 29     //Pin of the red lights below court
#define BLUE_LIGHTS_BELOW_PIN 31    //Pin of the blues lights below court
#define RED_LIGHTS_BASKET_PIN 11    //Pin of the red lights behind the basket

#define BUZZER_PIN 8                //Pin of the buzzer behind the basket
#define LASER_PIN 9                 //Pin of the LASER denoting a succesfull shot

#define BASKET_PHOTOSENSOR A0       //Pin of the photosensor to sense the interuption of the laser light on it, denoting in this way the success of a shot
#define LAUNCHER1_PHOTOSENSOR A1    //Pin of the photosensor from Lancher#1. When the ball has been placed in this lancher, its values decreases.
#define LAUNCHER2_PHOTOSENSOR A2    //Pin of the photosensor from Lancher#2. When the ball has been placed in this lancher, its values decreases.
#define LAUNCHER3_PHOTOSENSOR A3    //Pin of the photosensor from Lancher#3. When the ball has been placed in this lancher, its values decreases.
#define LAUNCHER4_PHOTOSENSOR A4    //Pin of the photosensor from Lancher#4. When the ball has been placed in this lancher, its values decreases.
#define LAUNCHER5_PHOTOSENSOR A5    //Pin of the photosensor from Lancher#5. When the ball has been placed in this lancher, its values decreases.
#define LAUNCHER6_PHOTOSENSOR A6    //Pin of the photosensor from Lancher#6. When the ball has been placed in this lancher, its values decreases.

//Constants to denote distinct phases of the game
#define STARTING_PHASE 0            //When the system boots      
#define SETTINGS_PHASE 1            //When user enters the setup subsystem 
#define GAMING_PHASE 2              //When users are ready to play
#define PLAYER_TIMER 3              //When a user is playing

//Constants to denote distinct screens of the game
#define INIT_SCREEN -1              //When the system is at the beginning
#define SCR1_2    0                 //Distinct phases of the setup menu
#define SCR1_2_1  1
#define SCR1_2_2  2
#define SCR1_2_3  3
#define SCR1_2_4  4
#define SCR1_2_5  5
#define PLAYING_SCREEN 6

#define RED 1                       //Turn on red push light
#define WHITE 2                     //Turn on white light
#define YELLOW 4                    //Turn on yellow light

#define RED_PUSH_BUTTON_LED_PIN 6     //Pin of the Red push button led
#define WHITE_PUSH_BUTTON_LED_PIN 7   //Pin of the Red push button led
#define YELLOW_PUSH_BUTTON_LED_PIN 5  //Pin of the Red push button led

#define LENGTH(a) (sizeof(a) / sizeof(*a)) //length of a string

#define BASKET_SUCCESS_THRESHOLD 100       //Difference in basket photosensor to denote the success of a shot

//Variables for texts to be displayed on LCD Screen
char *scr1[]     = {"Yellow: Play", "White : Settings"};
char *scr1_2[]   = {"Play               ", "Player shots      >","Player time       >",  "Player#1 Team     >", "Player#2 Team     >", "Buzzer            >"};
char *scr1_2_1[] = {"10"};
char *scr1_2_2[] = {"2:00"};
char *scr1_2_5[] = {"On", "Off"};

char OptionsSettings[20] = "W:Down  R:SEL  Y:Up";
char OptionsGame[20];

// Variable used for millis debounce
long TimeOfLastDebounce[3];// = {0,0,0};  // holds the last time the switch was pressed
int ButtonState[3];// = {0,0,0};    
long DelayofDebounce = 100;  // amount of time that needs to be experied between presses

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address

char *NBATeams[]={"Hawks","Celtics","Nets","Hornets","Bulls","Cavaliers","Mavericks","Nuggets","Pistons","Warriors","Rockets","Pacers","Clippers",
"Lakers","Grizzlies","Heat","Bucks","Timberwolves","Pelicans","Knicks","Thunder","Magic","76ers","Suns","Blazers","Kings","Spurs","Raptors","Jazz","Wizards"};

char LCDLineBuffer[20];
char EmptyBuffer[] = "                   ";

//Global game's parameters
int PlayerShots;        //10 shots per player
int PlayerTime;         //Inital Player Time in seconds
int InitPlayerTime;     //Player Time in seconds
int PlayerATeamInd;     //Index of the 1st player team in NBATeams matrix 
int PlayerBTeamInd;     //Index of the 2nd player team in NBATeams matrix
int PlayerAScore;       
int PlayerBScore;
int PlayerATime;
int PlayerBTime;
int PlayerAShots;
int PlayerBShots;
int SelectedPlayer;

int LauncherPsValue[6], LauncherPrevPsValue[6];
int Diffs[6];

int Points[6]={3,3,2,1,4,6};              //Launchers points on succesful shots
int SelectedLauncher;
int BasketPsValue, BasketPrevPsValue;

boolean BuzzerAtEnd;
boolean CountingDownTime;

char **CurrentScreen;
int CurrentScreenId;
int ProgramState;
int StartDisplayedOption;
int LastDisplayedOption;
int SelectedOption;
int UpLimit;
boolean Verbose;
int Played;

//Display options on the LCD screen during setup phase of the game
void UpdateDisplaySettings(boolean info) {
   int k,m=0;
   for (k=StartDisplayedOption;k<=LastDisplayedOption;k++) {
    lcd.setCursor(0,m++);
    sprintf(LCDLineBuffer, "%s", EmptyBuffer);
    sprintf(LCDLineBuffer, "%-19s", CurrentScreen[k]);
    if (k==SelectedOption) {lcd.print("*");lcd.print(LCDLineBuffer);}
    else {lcd.print(" ");lcd.print(LCDLineBuffer);}
   }
   if (m<4) 
    for (k=m;k<4;k++) {
      lcd.setCursor(0,k);
      lcd.print(EmptyBuffer);
    }
    if (info) {
      lcd.setCursor(0,k);
      lcd.print(OptionsSettings);
      PushButtonLights(RED+WHITE+YELLOW);
    } else {
      PushButtonLights(WHITE+YELLOW);
    }
}

//Arrange the necessary push buttons lights
void PushButtonLights(int arg) { 
    digitalWrite(WHITE_PUSH_BUTTON_LED_PIN, arg&WHITE);
    digitalWrite(YELLOW_PUSH_BUTTON_LED_PIN, arg&YELLOW);
    digitalWrite(RED_PUSH_BUTTON_LED_PIN, arg&RED);
}

//Display all the necessary info during game playing e.g remaining time and shots, current score e.t.c.
void UpdateDisplayGaming(boolean ctrl) {
  if (SelectedPlayer>0) {
    PlayerATime = PlayerTime;
    sprintf(LCDLineBuffer, "Time:%-3d, Shots:%-3d", PlayerATime, PlayerAShots);
    lcd.setCursor(0,0);
    lcd.print(LCDLineBuffer);
    sprintf(LCDLineBuffer, "*%-13s:%-3d", NBATeams[PlayerATeamInd], PlayerAScore);
    lcd.setCursor(0,1);
    lcd.print(LCDLineBuffer);
    lcd.setCursor(0,2);
    sprintf(LCDLineBuffer, " %-13s:%-3d", NBATeams[PlayerBTeamInd], PlayerBScore);
    lcd.print(LCDLineBuffer);
  } else {
    PlayerBTime = PlayerTime;
    sprintf(LCDLineBuffer, "Time:%-3d, Shots:%-3d", PlayerBTime, PlayerBShots);
    lcd.setCursor(0,0);
    lcd.print(LCDLineBuffer);
    sprintf(LCDLineBuffer, " %-13s:%-3d", NBATeams[PlayerATeamInd], PlayerAScore);
    lcd.setCursor(0,1);
    lcd.print(LCDLineBuffer);
    lcd.setCursor(0,2);
    sprintf(LCDLineBuffer, "*%-13s:%-3d", NBATeams[PlayerBTeamInd], PlayerBScore);
    lcd.print(LCDLineBuffer);  
  }
  if (ctrl) {
      if (Played==0) {
        sprintf(LCDLineBuffer, "W:Start    Y:Change");
        PushButtonLights(YELLOW+WHITE);
      } else if (Played==1) {
        sprintf(LCDLineBuffer, "W:Start            ");
        PushButtonLights(WHITE);
      } else {
        sprintf(LCDLineBuffer, "R:OK               ");
        PushButtonLights(RED);
      }
      lcd.setCursor(0,3); lcd.print(LCDLineBuffer);
      
  } else {
      if (Played==2) sprintf(LCDLineBuffer, "R:OK               ");
      else sprintf(LCDLineBuffer, "R:Abort               ");
      lcd.setCursor(0,3); lcd.print(LCDLineBuffer);
      PushButtonLights(RED);
  }
}

//Set game's parameters in their initial default values
void InitializeGame() {
  PlayerShots=10; //10 shots per player
  InitPlayerTime = PlayerTime=10; //in seconds
  PlayerATeamInd = 0;
  PlayerBTeamInd = 1;
  PlayerAScore=0;
  PlayerBScore=0;
  PlayerATime=PlayerTime;
  PlayerBTime=PlayerTime;
  PlayerAShots=PlayerShots;
  PlayerBShots=PlayerShots;
  BuzzerAtEnd=true;
  StartDisplayedOption = 0;
  LastDisplayedOption=1;
  SelectedOption = 0;
  ProgramState = STARTING_PHASE;
  CurrentScreenId = INIT_SCREEN;
  CurrentScreen = scr1;
  UpLimit=LENGTH(scr1);
  TimeOfLastDebounce[0] = TimeOfLastDebounce[1] = TimeOfLastDebounce[2] = 0;
  ButtonState[0] = ButtonState[1] = ButtonState[2] = 0;
  Verbose=false;
  Played=0;

  UpdateDisplaySettings(false);
  LauncherPrevPsValue[0] = analogRead(LAUNCHER1_PHOTOSENSOR);
  LauncherPrevPsValue[1] = analogRead(LAUNCHER2_PHOTOSENSOR);
  LauncherPrevPsValue[2] = analogRead(LAUNCHER3_PHOTOSENSOR);
  LauncherPrevPsValue[3] = analogRead(LAUNCHER4_PHOTOSENSOR);
  LauncherPrevPsValue[4] = analogRead(LAUNCHER5_PHOTOSENSOR);
  LauncherPrevPsValue[5] = analogRead(LAUNCHER6_PHOTOSENSOR);
}


//Enables timer5 of Arduino Mega to countdown the remaining time of each user.
void SetupTimer5() {
  cli();//stop interrupts

  //set timer5 interrupt at 1Hz = call ISR every 1 sec
  TCCR5A = 0;// set entire TCCR1A register to 0
  TCCR5B = 0;// same for TCCR1B
  TCNT5  = 0;//initialize counter value to 0
  // set compare match register for 1hz increments
  OCR5A = 15624;// = (16*10^6) / (1*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR5B |= (1 << WGM12);
  // Set CS12 and CS10 bits for 1024 prescaler
  TCCR5B |= (1 << CS12) | (1 << CS10);  
  // disable timer compare interrupt
  TIMSK5 |= (1 << OCIE5A);
  
  sei();//allow interrupts
}

//Deactivate timer5 of Arduino Mega
void DeactivateTimer() {
     TCCR5A = 0;
     TCCR5B = 0;                      // stop timer
     TIMSK5 = (0 << OCIE5A);          // cancel timer interrupt
}

void setup() {
    Serial.begin(9600);
    lcd.begin(20,4);
    lcd.backlight();
    //lcd.noBacklight();
    pinMode(UP_BTN_PIN, INPUT_PULLUP); 
    pinMode(DOWN_BTN_PIN, INPUT_PULLUP);
    pinMode(SETUP_BTN_PIN, INPUT_PULLUP);
    
    pinMode(BASKET_PHOTOSENSOR, INPUT);
    pinMode(LAUNCHER1_PHOTOSENSOR, INPUT);
    pinMode(LAUNCHER2_PHOTOSENSOR, INPUT);
    pinMode(LAUNCHER3_PHOTOSENSOR, INPUT);
    pinMode(LAUNCHER4_PHOTOSENSOR, INPUT);
    pinMode(LAUNCHER5_PHOTOSENSOR, INPUT);
    pinMode(LAUNCHER6_PHOTOSENSOR, INPUT);
    
    pinMode(RED_PUSH_BUTTON_LED_PIN, OUTPUT);
    pinMode(WHITE_PUSH_BUTTON_LED_PIN, OUTPUT);
    pinMode(YELLOW_PUSH_BUTTON_LED_PIN, OUTPUT);
    pinMode(RED_LIGHTS_BELOW_PIN , OUTPUT);
    pinMode(BLUE_LIGHTS_BELOW_PIN , OUTPUT);
    pinMode(RED_LIGHTS_BASKET_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LASER_PIN, OUTPUT); 
    
    //digitalWrite(RED_LIGHTS_BELOW_PIN, HIGH);
    //digitalWrite(BLUE_LIGHTS_BELOW_PIN, HIGH);
    //digitalWrite(RED_LIGHTS_BASKET_PIN, HIGH);  
    //digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(LASER_PIN, HIGH);
    BasketPrevPsValue = analogRead(BASKET_PHOTOSENSOR);
    InitializeGame();
}

void loop() { 
    if (ProgramState==PLAYER_TIMER) {
      GetPhotoSensorsValues();
      CheckBasketSuccess();
      UpdateGamingScreen();
    }
    if (ButtonPressed(UP_BTN_PIN)) UpButtonHandler();
    if (ButtonPressed(DOWN_BTN_PIN)) DownButtonHandler();
    if (ButtonPressed(SETUP_BTN_PIN)) SetupButtonHandler();
}

ISR(TIMER5_COMPA_vect){//timer1 interrupt 1Hz
  PlayerTime--;
  if (PlayerTime==0) DeactivateTimer();
}

//Checks is a shot is succesful
void CheckBasketSuccess() {
  BasketPsValue = analogRead(BASKET_PHOTOSENSOR);
  if (abs(BasketPrevPsValue-BasketPsValue)>BASKET_SUCCESS_THRESHOLD) {
      if (SelectedPlayer>0) 
        PlayerAScore += Points[SelectedLauncher];
      else
        PlayerBScore += Points[SelectedLauncher];
  //  digitalWrite(BUZZER_PIN, HIGH); delay(500);digitalWrite(BUZZER_PIN, LOW);
  //  Serial.print("BS=");Serial.println(BasketPsValue);
  }
  BasketPrevPsValue =BasketPsValue;
  
  //Serial.println(BasketPsValue);
}

//Get light's values from Launcher to verify if there is a ball on any of them. If there is a ball, then SelectedLauncher variable denotes this launcher
void GetPhotoSensorsValues() {
  int launcher;
    int k;
      BasketPsValue = analogRead(BASKET_PHOTOSENSOR);
      LauncherPsValue[0] = analogRead(LAUNCHER1_PHOTOSENSOR);
      LauncherPsValue[1] = analogRead(LAUNCHER2_PHOTOSENSOR);
      LauncherPsValue[2] = analogRead(LAUNCHER3_PHOTOSENSOR);
      LauncherPsValue[3] = analogRead(LAUNCHER4_PHOTOSENSOR);
      LauncherPsValue[4] = analogRead(LAUNCHER5_PHOTOSENSOR);
      LauncherPsValue[5] = analogRead(LAUNCHER6_PHOTOSENSOR);
      
      for (k=0;k<6;k++)
        Diffs[k] = abs(LauncherPsValue[k] - LauncherPrevPsValue[k]);
      
      launcher=BallOnLauncher();
      if (launcher!=-1) {
          if (LauncherPsValue[launcher] - LauncherPrevPsValue[launcher]>0) {
            SelectedLauncher = launcher;
            if (SelectedPlayer>0) {PlayerAShots--; PlayerAScore += Points[SelectedLauncher];}
            else {PlayerBShots--; PlayerBScore += Points[SelectedLauncher];}
          }   
      }
      if (Verbose) {
          Serial.print("BS=");Serial.print(BasketPsValue);Serial.print(",");
          Serial.print("L1=");Serial.print(Diffs[0]);Serial.print(",");
          Serial.print("L2=");Serial.print(Diffs[1]);Serial.print(",");
          Serial.print("L3=");Serial.print(Diffs[2]);Serial.print(",");
          Serial.print("L4=");Serial.print(Diffs[3]);Serial.print(",");
          Serial.print("L5=");Serial.print(Diffs[4]);Serial.print(",");
          Serial.print("L6=");Serial.print(Diffs[5]);Serial.println(",");
          delay(300);
      }
      for (k=0;k<6;k++)
        LauncherPrevPsValue[k] = LauncherPsValue[k];
}

#define BALL_DIFF 70      //Constant to verify the existence of a ball is on a specific Launcher
#define DIST_DIFF 5       //Constant to verify that the light on a photosensor is almost the same.

//returns the index of the user selected launcher to accomplish his next shot
int BallOnLauncher() {
  int k,count;
  int maxdiff, pos;
  
  maxdiff = -1;
  pos = -1;
  for (k=0;k<6;k++)
    if (Diffs[k]>maxdiff) {
      maxdiff = Diffs[k];
      pos = k;
    }
  if (Diffs[pos]>BALL_DIFF) {
     int count=0;
     for (k=0;k<6;k++) 
       if ((k!=pos) && (Diffs[k]>DIST_DIFF)) count++;
     if (count==0) return(pos);
     else return(-1);
  }
  return(-1);
}

//Updates LCD screen during game playing
void UpdateGamingScreen() {
  if (PlayerTime>0) {
    UpdateDisplayGaming(false);
  } else {
       Played++; 
       if (BuzzerAtEnd) digitalWrite(BUZZER_PIN, HIGH);
       digitalWrite(RED_LIGHTS_BASKET_PIN, HIGH);
       delay(1500);
       digitalWrite(BUZZER_PIN, LOW);
       digitalWrite(RED_LIGHTS_BASKET_PIN, LOW);
       SelectedPlayer = (-1)*SelectedPlayer;
       PlayerTime = InitPlayerTime;
       if (Played<2) {
           ProgramState=GAMING_PHASE;
           UpdateDisplayGaming(true);
       } 
  }
}

//Check is a push button attached in BtnPin is pressed using debounce
boolean ButtonPressed(int BtnPin) {
      int ind = BtnPin - 2;
      if (digitalRead(BtnPin) == LOW && ButtonState[ind] == 0) {
      // check if enough time has passed to consider it a switch press
      if ((millis() - TimeOfLastDebounce[ind]) > DelayofDebounce) {
        ButtonState[ind]=1;
        TimeOfLastDebounce[ind] = millis();
        return(true);
      }
    } else {
      if (ButtonState[ind] == 1 && digitalRead(BtnPin) == HIGH){
        ButtonState[ind]=0;
        return(false);
      }
    }
    return(false);
}

//Yellow push button handler
void UpButtonHandler() { //Yellow Push Button
  if (CurrentScreenId==INIT_SCREEN) {
    StartPlaying();
  } else if (ProgramState==GAMING_PHASE) {
      if (Played==0) SelectedPlayer=(-1)*SelectedPlayer;
      UpdateDisplayGaming(true);
  } else if (CurrentScreenId==SCR1_2_1) { //adjusting Players shots
         PlayerShots++;
         sprintf(scr1_2_1[0],"%-d",PlayerShots);
         UpdateDisplaySettings(true);  
  } else if (CurrentScreenId==SCR1_2_2) { //adjusting Players time
          PlayerTime+=10;
          int secs = PlayerTime%60;
          int mins = (int) (PlayerTime-secs)/60;
          sprintf(scr1_2_2[0],"%-d:%-d",mins,secs);
          UpdateDisplaySettings(true);
  } else if (ProgramState!=PLAYER_TIMER) {
          adjust_options(UP);
  }
}

//White push button handler
void DownButtonHandler() { //White Push Button
  if (ProgramState == STARTING_PHASE) {
    ProgramState = SETTINGS_PHASE;
    NextScreen();
  } else if (ProgramState==GAMING_PHASE) {
    ProgramState=PLAYER_TIMER;
    SetupTimer5();
  } else if (CurrentScreenId==SCR1_2_1) { //adjusting Players shots
      if (PlayerShots>1) {
          PlayerShots--;
          sprintf(scr1_2_1[0],"%-d",PlayerShots);
          UpdateDisplaySettings(true);
      }
  } else if (CurrentScreenId==SCR1_2_2) { //adjusting Players time
      if (InitPlayerTime>20) {
          InitPlayerTime-=10;
          int secs = InitPlayerTime%60;
          int mins = (InitPlayerTime-secs)/60;
          sprintf(scr1_2_2[0],"%-d:%-2d",mins,secs);
          UpdateDisplaySettings(true);
      }
  } else if (ProgramState!=PLAYER_TIMER) {
    adjust_options(DOWN);
  }
}

//Initializing Playing
void StartPlaying() {
    ProgramState = GAMING_PHASE;
    PlayerAScore=0;
    PlayerBScore=0;
    PlayerTime = InitPlayerTime;
    PlayerATime=PlayerTime;
    PlayerBTime=PlayerTime;
    PlayerAShots = PlayerShots;
    PlayerBShots = PlayerShots;
    SelectedPlayer=1;
    CurrentScreenId = PLAYING_SCREEN;
    UpdateDisplayGaming(true);
}

//Red push button handler
void SetupButtonHandler() { 
  if (ProgramState==PLAYER_TIMER) {
    //Reset 
    InitializeGame();
  } else if (CurrentScreenId==SCR1_2) {
      if (SelectedOption==0) {
          StartPlaying();
      } else NextScreen();
  } else {
    PrevScreen();
  }
}

//Adjust displayed options on a menu in the LCD screen
void adjust_options(int mvm) {
  if ((mvm == UP)&&(SelectedOption<UpLimit)) {
    SelectedOption++;
    if (SelectedOption>LastDisplayedOption) {
      if (StartDisplayedOption+2<UpLimit) { StartDisplayedOption++; LastDisplayedOption++;}
   } 
  }
    if ((mvm == DOWN)&&(SelectedOption>0)) {
    SelectedOption--;
    if (SelectedOption<StartDisplayedOption) {
      StartDisplayedOption--;
      LastDisplayedOption=StartDisplayedOption+2;
   } 
  }
  UpdateDisplaySettings(true);
}

//Determines which is the next setup screen according to the current setup screen
void NextScreen() {
    switch (CurrentScreenId) {
      case INIT_SCREEN:  
          if (ProgramState==SETTINGS_PHASE) {
               CurrentScreen = scr1_2;
               UpLimit=LENGTH(scr1_2)-1;
               SelectedOption = 0;
               StartDisplayedOption=0;
               LastDisplayedOption=2;
               CurrentScreenId = SCR1_2;
               UpdateDisplaySettings(true);     
          }  
        break;
      case SCR1_2: 
        switch (SelectedOption) {
            case 0: //play game
              StartPlaying();
              break;
            case 1: //Player Shots
                 CurrentScreen = scr1_2_1;
                 UpLimit=LENGTH(scr1_2_1)-1;
                 SelectedOption = 0;
                 StartDisplayedOption=0;
                 LastDisplayedOption=0;
                 CurrentScreenId = SCR1_2_1;
                 UpdateDisplaySettings(true); 
              break;
            case 2: //Player time
                 CurrentScreen = scr1_2_2;
                 UpLimit=LENGTH(scr1_2_2)-1;
                 SelectedOption = 0;
                 StartDisplayedOption=0;
                 LastDisplayedOption=0;
                 CurrentScreenId = SCR1_2_2;
                 UpdateDisplaySettings(true);
              break;
            case 3: //Player#1 Team
                 CurrentScreen = NBATeams;
                 UpLimit=LENGTH(NBATeams)-1;
                 SelectedOption = 0;
                 StartDisplayedOption=0;
                 LastDisplayedOption=2;
                 CurrentScreenId = SCR1_2_3;
                 UpdateDisplaySettings(true);
              break;
            case 4: //Player#2 Team
                 CurrentScreen = NBATeams;
                 UpLimit=LENGTH(NBATeams)-1;
                 SelectedOption = 0;
                 StartDisplayedOption=0;
                 LastDisplayedOption=2;
                 CurrentScreenId = SCR1_2_4;
                 UpdateDisplaySettings(true);            
              break;
            case 5:
                 CurrentScreen = scr1_2_5;
                 UpLimit=LENGTH(scr1_2_5)-1;
                 SelectedOption = 0;
                 StartDisplayedOption=0;
                 LastDisplayedOption=1;
                 CurrentScreenId = SCR1_2_5;
                 UpdateDisplaySettings(true);            
              break;
        }
        break;
  }
}

//Determines which is the previous setup screen according to the current setup screen
void PrevScreen() {
    switch (CurrentScreenId) {
      case SCR1_2: 
            ProgramState = STARTING_PHASE;
            CurrentScreenId = INIT_SCREEN;
            CurrentScreen = scr1;
            UpLimit=LENGTH(scr1);
            UpdateDisplaySettings(false);
          break;
      case SCR1_2_1:
               CurrentScreen = scr1_2;
               UpLimit=LENGTH(scr1_2)-1;
               SelectedOption = 0;
               StartDisplayedOption=0;
               LastDisplayedOption=2;
               CurrentScreenId = SCR1_2;
               UpdateDisplaySettings(true);
               break;
      case SCR1_2_2: 
               CurrentScreen = scr1_2;
               UpLimit=LENGTH(scr1_2)-1;
               SelectedOption = 0;
               StartDisplayedOption=0;
               LastDisplayedOption=2;
               CurrentScreenId = SCR1_2;
               UpdateDisplaySettings(true);      
               break;
      case SCR1_2_3:
          PlayerATeamInd = SelectedOption;
               CurrentScreen = scr1_2;
               UpLimit=LENGTH(scr1_2)-1;
               SelectedOption = 0;
               StartDisplayedOption=0;
               LastDisplayedOption=2;
               CurrentScreenId = SCR1_2;
               UpdateDisplaySettings(true);           
          break;
      case SCR1_2_4:
          PlayerBTeamInd = SelectedOption;
               CurrentScreen = scr1_2;
               UpLimit=LENGTH(scr1_2)-1;
               SelectedOption = 0;
               StartDisplayedOption=0;
               LastDisplayedOption=2;
               CurrentScreenId = SCR1_2;
               UpdateDisplaySettings(true);           
          break;
      case SCR1_2_5:    
          BuzzerAtEnd = (SelectedOption==0) ? true: false;
               CurrentScreen = scr1_2;
               UpLimit=LENGTH(scr1_2)-1;
               SelectedOption = 0;
               StartDisplayedOption=0;
               LastDisplayedOption=2;
               CurrentScreenId = SCR1_2;
               UpdateDisplaySettings(true);      
          break;
  } 
}

