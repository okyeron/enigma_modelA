#include "usb_names.h"

#define PRODUCT_NAME      {'e','n','i','g','m','a','m','i','d','i'}
#define PRODUCT_NAME_LEN   10
#define MANUFACTURER_NAME  {'e','n','i','g','m','a'}
#define MANUFACTURER_NAME_LEN 6

// Edit these lines to create your own name.  The length must
// match the number of characters in your custom name.

#define MIDI_NAME   {'e','n','i','g','m','a'}
#define MIDI_NAME_LEN  6


struct usb_string_descriptor_struct usb_string_product_name = {
  2 + PRODUCT_NAME_LEN * 2,
  3,
  PRODUCT_NAME
};

struct usb_string_descriptor_struct usb_string_manufacturer_name = {
  2 + MANUFACTURER_NAME_LEN * 2,
  3,
  MANUFACTURER_NAME
};

// Do not change this part.  This exact format is required by USB.

struct usb_string_descriptor_struct usb_string_product_name = {
        2 + MIDI_NAME_LEN * 2,
        3,
        MIDI_NAME
};