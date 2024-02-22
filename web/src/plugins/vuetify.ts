/**
 * plugins/vuetify.ts
 *
 * Framework documentation: https://vuetifyjs.com`
 */

// Styles
import 'vuetify/styles';

// Composables
import { createVuetify, ThemeDefinition } from 'vuetify';

// icons via @mdi/js so we only include the used
import { aliases, mdi } from 'vuetify/iconsets/mdi-svg';

const themeBottleFillerLight: ThemeDefinition = {
  dark: false,
  colors: {
    background: '#FFFFFF',
    surface: '#FFFFFF',
    primary: '#10b382',
    'primary-darken-1': '#107053',
    secondary: '#3eb86f',
    'secondary-darken-1': '#94791f',
    error: '#B00020',
    info: '#2196F3',
    success: '#4CAF50',
    warning: '#FB8C00',
  },
};

const themeBottleFillerDark: ThemeDefinition = {
  dark: true,
  colors: {
    background: '#000000',
    surface: '#242320',
    primary: '#10b382',
    'primary-darken-1': '#107053',
    secondary: '#3eb86f',
    'secondary-darken-1': '#94791f',
    error: '#B00020',
    info: '#2196F3',
    success: '#4CAF50',
    warning: '#FB8C00',
  },
};

// https://vuetifyjs.com/en/introduction/why-vuetify/#feature-guides
export default createVuetify({
  theme: {
    defaultTheme: 'dark',
    themes: {
      light: themeBottleFillerLight,
      dark: themeBottleFillerDark,
    },
  },
  icons: {
    defaultSet: 'mdi',
    aliases,
    sets: {
      mdi,
    },
  },
});
