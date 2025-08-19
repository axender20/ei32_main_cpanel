#include "utilities_realtime.h"

bool parseBinaryArray(const String& input, bool output[6]) {
  // Validar longitud exacta: [x,x,x,x,x,x] → 13 caracteres
  if (input.length() != 13) return false;

  // Validar formato
  if (input.charAt(0) != '[' || input.charAt(12) != ']') return false;
  for (int i = 0; i < 6; i++) {
    int valueIndex = 1 + i * 2;
    if (input.charAt(valueIndex) != '0' && input.charAt(valueIndex) != '1') return false;
    if (i < 5 && input.charAt(valueIndex + 1) != ',') return false;
    // Guardar en array booleano
    output[i] = (input.charAt(valueIndex) == '1');
  }
  return true;
}