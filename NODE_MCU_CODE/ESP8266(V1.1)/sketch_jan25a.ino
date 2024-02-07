/************************************************/
/*  Author  : ABDULLAH AYMAN                    */
/*  Date    : 25/1/2024  8:00PM                 */
/* topic    : WIFI + MQTT + DOWNLOAD + UART     */
/* version  : 1.4                               */
/************************************************/
//------Code------//

//------INCLUDES------//

#include <ESP8266WiFi.h>     // Include the ESP8266WiFi library
#include <PubSubClient.h>    // Include the PubSubClient library
#include <LittleFS.h>        // Include the LittleFS library

//--------------------//



//------START OF CODE FOR FILE HANDLING AND SENDING------//

// Forward declaration of the Node structure
struct Node;

struct Node {
  String data;
  Node* next;
};
const int switchPin = D1; // GPIO 5
static int lastSwitchState = HIGH; // Variable to store the previous switch state
String fileName = "APP2";
void readFile(String fileName);
// Function to create a linked list from file content
Node* createLinkedListFromFile(String filePath) {
  Node* head = NULL;  // Initialize the head of the linked list

  // Check if LittleFS is mounted
  if (!LittleFS.begin()) {
    //Serial.println("LittleFS Mount Failed");
    return head;  // Return NULL to indicate failure
  }

  // Open the file in read mode
  File file = LittleFS.open(filePath, "r");
  if (!file) {
    //Serial.println("Failed to open file for reading");
    return head;  // Return NULL to indicate failure
  }

  // Initialize the head of the linked list
  head = new Node();
  head->next = NULL;

  Node* current = head;  // Initialize the current node

  // Read the content of the file line by line
  while (file.available()) {
    String line = file.readStringUntil('\n');

    // Create a new node and save the line as data
    Node* newNode = new Node();
    newNode->data = line;
    newNode->next = NULL;

    // Link the new node to the current node
    current->next = newNode;
    current = newNode;  // Move to the new node
  }

  // Close the file
  file.close();

  return head;  // Return the head of the linked list
}

//for debugging
void printLinkedListUpToLine(Node* head, int lineNumber) {
  Node* current = head->next;  // Start from the first node (skip the dummy head)
  int currentLine = 1;

  while (current != nullptr && currentLine <= lineNumber) {
    if(currentLine==(lineNumber)){
      //Serial.println(current->data);
    }
    current = current->next;
    currentLine++;
  }
}




// Function to free the memory allocated for the linked list
void freeLinkedList(Node* head) {
  Node* current = head;
  Node* nextNode;

  while (current != nullptr) {
    nextNode = current->next;
    delete current;
    current = nextNode;
  }
}

//function to create a directory in the fileSystem
String createDirectory(String directoryName) {
  // Check if LittleFS is mounted
  if (!LittleFS.begin()) {
    //Serial.println("LittleFS Mount Failed");
    return "";  // Return an empty string to indicate failure
  }

  // Create the full path by concatenating the root path and the directory name
  String directoryPath = "/" + directoryName;

  // Check if the directory already exists
  if (LittleFS.exists(directoryPath)) {
    //Serial.println("\nDirectory already exists");
    return directoryPath;
  }

  // Create the directory
  if (LittleFS.mkdir(directoryPath)) {
   // Serial.println("\nDirectory created successfully");
    return directoryPath;
  } else {
    //Serial.println("\nFailed to create directory");
    return "";  // Return an empty string to indicate failure
  }
}

bool createFile(String filePath) {
  // Check if LittleFS is mounted
  if (!LittleFS.begin()) {
    //Serial.println("LittleFS Mount Failed");
    return false;  // Return false to indicate failure
  }

  // Check if the file already exists
  if (LittleFS.exists(filePath)) {
    //Serial.println("File already exists");
    return false;  // Return false to indicate failure
  }

  // Create the file
  File file = LittleFS.open(filePath, "w");
  if (file) {
    file.close();
    //Serial.println("File created successfully");
    return true;  // Return true to indicate success
  } else {
    //Serial.println("Failed to create file");
    return false;  // Return false to indicate failure
  }
}

bool DeleteFile(String filePath)
{
  //check if the file is existing
  if(LittleFS.exists(filePath))
  {
    //Serial.println("File Was Found");
    //delete file at the specified path
    return (LittleFS.remove(filePath));
  }
  else
  {
    //Serial.println("File Was Not Found");
    return false;
  }
}
/*
void sendLinkedListDataThroughSPI(Node* head) {
  Node* current = head->next;  // Start from the first node (skip the dummy head)
  int q=0;
  while(current->next!=NULL){
    q=0;
    while (q<10) {
      SPI.begin();
      delay(100);
      
      String dataToSend = current->data;
  
      // Convert the String to a character array (c_str())
      const char* charArray = dataToSend.c_str();
  
      // Send data through SPI
      for (int i = 0; i < dataToSend.length(); i++) {
        SPI.transfer(charArray[i]);
        delay(10);
      }
  
      // Send a newline character as a delimiter between lines
      
      q+=1;
      //
      
    }
    SPI.end();
    current = current->next;
  }
  

}
*/


void sendLinkedListDataThroughUART(Node* head) {
    //digitalWrite(D1, HIGH);
    //delay(500);
    
    Node* current = head->next;  // Start from the first node (skip the dummy head)
  while(current->next!=NULL){
      delay(100);
      
      String dataToSend = current->data;
  
      // Convert the String to a character array (c_str())
      const char* charArray = dataToSend.c_str();
  
      // Send data through USART
      for (int i = 0; i < dataToSend.length(); i++) {
        Serial.write(charArray[i]);
      }
      Serial.write('\n');
      // Wait for acknowledgment
      while (!Serial.available()) {
        // Wait for data to be available
        delay(50);
      }

      // Read acknowledgment
      String ack = Serial.readStringUntil('\n');

      // Check if acknowledgment is received
      if (ack == "ACK") {
        //Serial.println("ACK received, sending next line...");
        current = current->next;
      }
      current = current->next;
    
  }
}

//------END OF CODE FOR FILE HANDLING AND SENDING------//



//------START OF CODE FOR WIFI CONNECTION AND DATA DOWNLOAD------//

const char* ssid = "Abdallah Ayman";
const char* password = "ConnectMeNow1122@";

const char* mqtt_server = "192.168.1.3";
const char* mqtt_topic = "example/fota";

WiFiClient espClient;
PubSubClient client(espClient);
bool TRANSMISSION_COMPLETE = false;
// Function prototypes
void listFiles();
void readFile(String fileName);
void formatLittleFS(); // New function prototype
void callback(char* topic, byte* payload, unsigned int length);
// File name

//File to be received Size
unsigned int SIZE_TO_BE_RECEIVED = 0;
unsigned int SEND_COUNT = 0;
//the function that is called when there's a new message on the topic
void callback(char* topic, byte* payload, unsigned int length) {
  
  //Serial.println("Inside callback function");
  
    // Create and open the download file in append mode
    File downloadFile = LittleFS.open("/" + fileName, "a");
    if (!downloadFile) {
      //Serial.println("Failed to create/download file");
      return;
    }

    if (String(topic) == mqtt_topic) {
    // Write the payload to the end of the new file
    downloadFile.write(payload, length);
    downloadFile.close();
    //List files in LittleFS
    //listFiles();
    // Check if the message indicates file transmission completion
    if (strcmp(topic, mqtt_topic) == 0) {
      /*
        String payloadStr = String((char*)payload);

        // Check if the message contains the "Done" signal
        if (payloadStr == "Done")
        {
            TRANSMISSION_COMPLETE = true;
        } 
        else 
        {
        // Check if the message contains the "Size:" signal
            if (payloadStr.startsWith("Size:"))
            {
                // Extract the size value after "Size:"
                int sizeIndex = payloadStr.indexOf(':') + 1;
                String sizeStr = payloadStr.substring(sizeIndex);
                // Convert the size string to an integer
                int fileSize = sizeStr.toInt();
                SIZE_TO_BE_RECEIVED=fileSize;
                //Serial.print("Received file size: ");
                //Serial.println(fileSize);
                // Add your code to compare fileSize with the actual size of the file
            }
        }
        */
  }

  }
}
// functionto reconnect to the MQTT server
void reconnect() {
  while (!client.connected()) {
    //Serial.print("Attempting connection...");
    if (client.connect("ESP8266Client")) {
      //Serial.println("Connected to MQTT Server");
      client.subscribe(mqtt_topic);
    } else {
      //Serial.print("Failed, rc=");
      //Serial.print(client.state());
      //Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

//function to list all files in the file system
void listFiles() {
  Serial.println("Listing files:");
  Dir dir = LittleFS.openDir("/");
  while (dir.next()) {
    Serial.print(dir.fileName());
    File f = dir.openFile("r");
    Serial.print(" - Size: ");
    Serial.println(f.size());
    f.close();
  }
  Serial.println("File list complete\n");
}

// function to read a file from the File System with a specified path
void readFile(String fileName) {
  Serial.println("Reading file: " + fileName);
  File file = LittleFS.open(fileName, "r");
  if (file) {
    while (file.available()) {
      Serial.write(file.read());
    }
    file.close();
    Serial.println("\nFile read complete\n");
  } else {
    Serial.println("Failed to open file for reading\n ");
  }
}

//function to Format the littleFS
void formatLittleFS() {
  //Serial.println("Formatting LittleFS...");
  LittleFS.format();
  //Serial.println("LittleFS formatted");
}

//------END OF CODE FOR WIFI CONNECTION AND DATA DOWNLOAD------//




bool checkSwitchState() {
  static unsigned long lastDebounceTime = 0; // Variable to store the last time the switch changed state
  const unsigned long debounceDelay = 50; // Debounce time in milliseconds
  

  int switchState = digitalRead(switchPin); // Read the current state of the switch

  if (switchState != lastSwitchState) {
    // Reset the debounce timer
    lastDebounceTime = millis();
  }

  // Check if the switch state has been stable for the debounce delay
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // If the switch state has changed and remained stable for the debounce delay
    if (switchState != lastSwitchState) {
      // Update the last switch state
      lastSwitchState = switchState;

      // Check if the switch is pressed (LOW)
      if (switchState == LOW) {
        // Switch is pressed, do something
        Serial.println("Switch pressed");
        return true;
      }
    }
  }
  

  // Store the current switch state for the next iteration
  lastSwitchState = switchState;
  return false;
}





void setup() {
  pinMode(switchPin, INPUT_PULLUP); // Set the switch pin as input with internal pull-up resistor
    // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.println("Connecting to WiFi...");
  }
  //Serial.println("Connected to WiFi");

  // Initialize LittleFS
  if (!LittleFS.begin()) {
    //Serial.println("LittleFS initialization failed!");
    return;
  }

  // Uncomment the line below to format LittleFS on startup
   formatLittleFS();
  
  // Connect to MQTT Broker
  client.setServer(mqtt_server, 1883);
  client.setCallback(&callback);
  
  while (!client.connected()) {
    if (client.connect("ESP8266Client")) {
      //Serial.println("Connected to MQTT Broker");
      client.subscribe(mqtt_topic);
    } else {
      //Serial.print("Failed, rc=");
      //Serial.print(client.state());
      //Serial.println(" Retrying in 5 seconds...");
      delay(500);
    }
  }

}

void loop() {
    if (!client.connected()) {
      reconnect();
    }
    String message = "Hello, MQTT!";
    client.publish("ESP8266", message.c_str());
    client.loop();

    // Open the file for reading
  File file = LittleFS.open("/" + fileName, "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }


  // Read the file content
  while (file.available()) {
    // Move the file pointer to the end of the file
  if (!file.seek(0, SeekEnd)) {
    Serial.println("Failed to move file pointer to the end");
    file.close();
    return;
  }

  // Find the position of the last newline character
  long newlinePos = file.position() - 1;
  while (newlinePos >= 0 && file.read() != '\n') {
    newlinePos--;
    file.seek(newlinePos);
  }

  // Read the last line from the file
  String lastLine = file.readStringUntil('\n');

  // Check if the last line contains "Done"
  if (lastLine.indexOf("Done") != -1)
    {
    Serial.println("Last line contains 'Done'");
    Serial.begin(115200);
      delay(50);
      if(SEND_COUNT<1)
      {
        while(!Serial.available()){
          Serial.println("Waitiing for start condition");
          delay(100);
        }
        String strt = Serial.readStringUntil('\n');

      // Check if acknowledgment is received
        if (strt == "START") {
            //Serial.println("ACK received, sending next line...");
            TRANSMISSION_COMPLETE = true;
            Node* linkedList = createLinkedListFromFile(("/" + fileName));
            sendLinkedListDataThroughUART(linkedList);
            SEND_COUNT+=1;
    
            // Free the memory allocated for the linked list
            freeLinkedList(linkedList);
            break; // Stop reading the file once "Done" is found
      }
          //Serial.println("Found 'Done' in file");
          
      }
    }
    else 
    {
    Serial.println("Last line does not contain 'Done'");
    }
    /*
    String line = file.readStringUntil('\n');
    // Check if the line contains "Done"
    if (line.indexOf("Done") != -1) {
      Serial.begin(115200);
      delay(50);
      if(SEND_COUNT<1)
      {
        while(!Serial.available()){
          Serial.println("Waitiing for start condition");
          delay(100);
        }
        String strt = Serial.readStringUntil('\n');

      // Check if acknowledgment is received
        if (strt == "START") {
            //Serial.println("ACK received, sending next line...");
            TRANSMISSION_COMPLETE = true;
            Node* linkedList = createLinkedListFromFile(("/" + fileName));
            sendLinkedListDataThroughUART(linkedList);
            SEND_COUNT+=1;
    
            // Free the memory allocated for the linked list
            freeLinkedList(linkedList);
            break; // Stop reading the file once "Done" is found
      }
          //Serial.println("Found 'Done' in file");
          
      }
      
    }
    */
  }
  if( SEND_COUNT == 1 )
  {
      if(checkSwitchState()==true)
      {
        SEND_COUNT=0;
      }else
      {
        SEND_COUNT+=1;
      }
      
      Serial.end();
      
      
  }
  
  

  // Close the file
  file.close();
    
  // Add more debug statements or code here

  //delay(1000);  // Add a delay to avoid overwhelming the serial output
}
