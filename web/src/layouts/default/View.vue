<script lang="ts" setup>
import { mdiBottleWineOutline, mdiKnob, mdiWifi, mdiWrenchCogOutline } from '@mdi/js';
import { ref } from 'vue';

const drawer = ref(true);
const linksBrewing = ref([
  [mdiKnob, 'Control', 'control'],
]);

const linksSettings = ref([
  [mdiWifi, 'Wifi Settings', 'wifiSettings'],
  [mdiBottleWineOutline, 'Filler Settings', 'fillerSettings'],
  [mdiWrenchCogOutline, 'System Settings', 'systemSettings'],
]);

const version = ref<string>(import.meta.env.VITE_APP_VERSION);

if (import.meta.env.MODE === 'development') {
  version.value = `${import.meta.env.VITE_APP_VERSION}_dev`;
}

</script>

<template>
  <v-app>
    <v-app-bar app color="primary">
      <v-app-bar-nav-icon @click="drawer = !drawer" />
      <v-toolbar-title>ESP Bottle Filler</v-toolbar-title>
      <v-spacer />
      <h5 class="mr-10">Version: {{version}}</h5>
    </v-app-bar>

    <v-navigation-drawer v-model="drawer">
      <v-list>

        <v-list-subheader>Brewing</v-list-subheader>
        <v-list-item v-for="[icon, text, route] in linksBrewing" :key="icon" link :to="route">
          <template v-slot:prepend>
            <v-icon>{{ icon }}</v-icon>
          </template>
          <v-list-item-title>{{ text }}</v-list-item-title>
        </v-list-item>

        <v-list-subheader>Settings</v-list-subheader>
        <v-list-item v-for="[icon, text, route] in linksSettings" :key="icon" link :to="route">
          <template v-slot:prepend>
            <v-icon>{{ icon }}</v-icon>
          </template>
          <v-list-item-title>{{ text }}</v-list-item-title>
        </v-list-item>

      </v-list>

    </v-navigation-drawer>

    <v-main style="min-height: 300px;">
      <router-view />
    </v-main>
  </v-app>
</template>
