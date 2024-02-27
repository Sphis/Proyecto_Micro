/**
 * @file ServoVoice.ino
 * @author Kevin Campos C.
 * @author Josué Salmerón C.
 * @brief En este código se muestra el resultado generado por la página Edge Impulse acá se hizo un entrenamiento de reconocimiento de voz
 * con 4 muestras que forma un conjunto de 216 grabaciones para realizar el entrenamiento. Dos keywords (abrir, cierra), las restantes es 
 * sonido desconocido y ruido, estas muestras se tomaron de la misma página. Hecho lo anterior se realizó la clasificación a partir de etiquetas
 *  donde se obtuvo un 91% de accuracy. 
 * @version 0.1
 * @date 2024-02-19
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <Servo.h>

Servo myservo;  // create servo object to control a servo
// twelve servo objects can be created on most boards

int pos = 0;    // variable to store the servo position

///////////////////////////////// Reconocimiento de voz /////////////////////////////////
#define EIDSP_QUANTIZE_FILTERBANK   0

/**
 * Define the number of slices per model window. E.g. a model window of 1000 ms
 * with slices per model window set to 4. Results in a slice size of 250 ms.
 * For more info: https://docs.edgeimpulse.com/docs/continuous-audio-sampling
 */
#define EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW 4

/*
 ** NOTE: If you run into TFLite arena allocation issue.
 **
 ** This may be due to may dynamic memory fragmentation.
 ** Try defining "-DEI_CLASSIFIER_ALLOCATION_STATIC" in boards.local.txt (create
 ** if it doesn't exist) and copy this file to
 ** `<ARDUINO_CORE_INSTALL_PATH>/arduino/hardware/<mbed_core>/<core_version>/`.
 **
 ** See
 ** (https://support.arduino.cc/hc/en-us/articles/360012076960-Where-are-the-installed-cores-located-)
 ** to find where Arduino installs cores on your machine.
 **
 ** If the problem persists then there's not enough memory for this model and application.
 */

/* Includes ---------------------------------------------------------------- */
#include <PDM.h>
#include <Prueba_2_inferencing.h>
// Configuracion RGB
#define RED     22
#define BLUE    24
#define GREEN   23
#define LED_PWR 25
/** Audio buffers, pointers and selectors */
typedef struct {
    signed short *buffers[2];
    unsigned char buf_select;
    unsigned char buf_ready;
    unsigned int buf_count;
    unsigned int n_samples;
} inference_t;

static inference_t inference;
static bool record_ready = false;
static signed short *sampleBuffer;
static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
static int print_results = -(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW);
int FLAG_GATE=0;
/**
 * @brief      Arduino setup function
 */
void setup()
{
    Serial.begin(9600);
    myservo.attach(10);  // attaches the servo on pin 9 to the servo object

    // put your setup code here, to run once:
    pinMode(RED, OUTPUT);
    pinMode(BLUE, OUTPUT);
    pinMode(GREEN, OUTPUT);
    pinMode(LED_PWR, OUTPUT);
    pinMode(2, OUTPUT); // Etiquete de cerrado
    pinMode(3, OUTPUT); // Etiqueta de abierto
    Serial.begin(115200);
    Serial.println("Edge Impulse Inferencing Demo");

    // summary of inferencing settings (from model_metadata.h)
    ei_printf("Inferencing settings:\n");
    ei_printf("\tInterval: %.2f ms.\n", (float)EI_CLASSIFIER_INTERVAL_MS);
    ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    ei_printf("\tSample length: %d ms.\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16);
    ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) /
                                            sizeof(ei_classifier_inferencing_categories[0]));

    run_classifier_init();
    if (microphone_inference_start(EI_CLASSIFIER_SLICE_SIZE) == false) {
        ei_printf("ERR: Could not allocate audio buffer (size %d), this could be due to the window length of your model\r\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT);
        return;
    }
}

/**
 * @brief En este loop se implementa la apertura y cerradura de la puerta por medio de voz. Cuando el mcu reconoce la frecuencia de la palabra cierra o abrir
 * sucederá que se girará 90 grados para cerrar o abir la puerta. Importante mencionar que al mcu se le debe tener paciencia ya que algunas veces se activa  
 * al escuchar alguna frecuencia similar a las dos palabras claves. Esto se debe a que la cantidad de muestras quizás no fue la suficiente y esto porque por 
 * cuestiones de software la plataforma Edge Impulse no era capaz de procesar datos. No obstante, en general todo funciona correctamente. 
 * @params: FLAG: señal que indica cuando la puerta está abierta o cerrada.
 */
void loop()
{
    bool m = microphone_inference_record();
    if (!m) {
        ei_printf("ERR: Failed to record audio...\n");
        return;
    }

    signal_t signal;
    signal.total_length = EI_CLASSIFIER_SLICE_SIZE;
    signal.get_data = &microphone_audio_signal_get_data;
    ei_impulse_result_t result = {0};

    EI_IMPULSE_ERROR r = run_classifier_continuous(&signal, &result, debug_nn);
    if (r != EI_IMPULSE_OK) {
        ei_printf("ERR: Failed to run classifier (%d)\n", r);
        return;
    }
// CIERRA    
  if (result.classification[0].value >= 0.85) {
    // TEST, flag =0, esta abierto.
    if(FLAG_GATE == 0){
      // goes from 0 degrees to 90 degrees
      for (pos = 90; pos > 0; pos -= 1) {
        Serial.println(pos);
        myservo.write(pos);              // tell servo to go to position in variable 'pos'
        delay(10);                      // waits 10ms for the servo to reach the position
        FLAG_GATE=1;
      }
  
    }
    digitalWrite(GREEN, HIGH);// SEÑAL DE CIERRA
    digitalWrite(BLUE, LOW); // SEÑAL ABRIR EN BAJO
    digitalWrite(RED, LOW); //  SEÑAL DE CIERRA EN BAJO
    digitalWrite(2, HIGH); // LED SEÑAL DE CIERRA EN ALTO
    digitalWrite(3, LOW); // LED SEÑAL DE CIERRA EN BAJO
  }
// ABRIR  
  else if (result.classification[1].value >= 0.85) {
    // TEST
    if(FLAG_GATE==1){
      // goes from 90 degrees to 0 degrees
      for (pos = 0; pos <= 90; pos += 1) {
        Serial.println(pos);
        myservo.write(pos);              // tell servo to go to position in variable 'pos'
        delay(10);
        FLAG_GATE=0;
      }
    }
    digitalWrite(BLUE, HIGH); // INDICADOR DE SEÑAL ABRIR EN ALTO
    digitalWrite(3, HIGH); // LED DE SEÑAL ABRIR EN ALTO
    digitalWrite(2, LOW); // LED DE SEÑAL DE CIERRA EN BAJO
    //
    digitalWrite(GREEN, LOW);
    digitalWrite(RED, LOW);
  }
  // SONIDO EXTERNO
  else if (result.classification[2].value >= 0.98) {
    digitalWrite(BLUE, HIGH);
    digitalWrite(GREEN, HIGH);
    digitalWrite(RED, HIGH);
  }
  // RUIDO EXTERNO
  else if (result.classification[3].value >= 0.98) {
    digitalWrite(LED_PWR, LOW);
  }
    if (++print_results >= (EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW)) {
        // print the predictions
        ei_printf("Predictions ");
        ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)",
            result.timing.dsp, result.timing.classification, result.timing.anomaly);
        ei_printf(": \n");
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
            ei_printf("    %s: %.5f\n", result.classification[ix].label,
                      result.classification[ix].value);
        }
#if EI_CLASSIFIER_HAS_ANOMALY == 1
        ei_printf("    anomaly score: %.3f\n", result.anomaly);
#endif

        print_results = 0;
    }
}

/**
 * @brief      PDM buffer full callback
 *             Get data and call audio thread callback
 */
static void pdm_data_ready_inference_callback(void)
{
    int bytesAvailable = PDM.available();

    // read into the sample buffer
    int bytesRead = PDM.read((char *)&sampleBuffer[0], bytesAvailable);

    if (record_ready == true) {
        for (int i = 0; i<bytesRead>> 1; i++) {
            inference.buffers[inference.buf_select][inference.buf_count++] = sampleBuffer[i];

            if (inference.buf_count >= inference.n_samples) {
                inference.buf_select ^= 1;
                inference.buf_count = 0;
                inference.buf_ready = 1;
            }
        }
    }
}

/**
 * @brief      Init inferencing struct and setup/start PDM
 *
 * @param[in]  n_samples  The n samples
 *
 * @return     { description_of_the_return_value }
 */
static bool microphone_inference_start(uint32_t n_samples)
{
    inference.buffers[0] = (signed short *)malloc(n_samples * sizeof(signed short));

    if (inference.buffers[0] == NULL) {
        return false;
    }

    inference.buffers[1] = (signed short *)malloc(n_samples * sizeof(signed short));

    if (inference.buffers[1] == NULL) {
        free(inference.buffers[0]);
        return false;
    }

    sampleBuffer = (signed short *)malloc((n_samples >> 1) * sizeof(signed short));

    if (sampleBuffer == NULL) {
        free(inference.buffers[0]);
        free(inference.buffers[1]);
        return false;
    }

    inference.buf_select = 0;
    inference.buf_count = 0;
    inference.n_samples = n_samples;
    inference.buf_ready = 0;

    // configure the data receive callback
    PDM.onReceive(&pdm_data_ready_inference_callback);

    PDM.setBufferSize((n_samples >> 1) * sizeof(int16_t));

    // initialize PDM with:
    // - one channel (mono mode)
    // - a 16 kHz sample rate
    if (!PDM.begin(1, EI_CLASSIFIER_FREQUENCY)) {
        ei_printf("Failed to start PDM!");
    }

    // set the gain, defaults to 20
    PDM.setGain(127);

    record_ready = true;

    return true;
}

/**
 * @brief      Wait on new data
 *
 * @return     True when finished
 */
static bool microphone_inference_record(void)
{
    bool ret = true;

    if (inference.buf_ready == 1) {
        ei_printf(
            "Error sample buffer overrun. Decrease the number of slices per model window "
            "(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW)\n");
        ret = false;
    }

    while (inference.buf_ready == 0) {
        delay(1);
    }

    inference.buf_ready = 0;

    return ret;
}

/**
 * Get raw audio signal data
 */
static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr)
{
    numpy::int16_to_float(&inference.buffers[inference.buf_select ^ 1][offset], out_ptr, length);

    return 0;
}

/**
 * @brief      Stop PDM and release buffers
 */
static void microphone_inference_end(void)
{
    PDM.end();
    free(inference.buffers[0]);
    free(inference.buffers[1]);
    free(sampleBuffer);
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_MICROPHONE
#error "Invalid model for current sensor."
#endif



///////////////////////////////// Servo  /////////////////////////////////
