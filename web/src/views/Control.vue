<script lang="ts" setup>
import { inject, onMounted, ref } from 'vue';
import FillerControl from '@/components/FillerControl.vue';
import WebConn from '@/helpers/webConn';
import { IFillerConfig } from '@/interfaces/IFillerConfig';
import { useAppStore } from '@/store/app';
import { IFillerRuntimeConfig } from '@/interfaces/IFillerRuntimeConfig';

const webConn = inject<WebConn>('webConn');

const appStore = useAppStore();

const fillerConfigs = ref<Array<IFillerConfig>>([]);

const getData = async () => {
  const requestData = {
    command: 'GetFillerSettings',
    data: null,
  };

  const apiResult = await webConn?.doPostRequest(requestData);

  if (apiResult === undefined || apiResult.success === false) {
    return;
  }

  fillerConfigs.value = apiResult.data;
};

// set runtime settings, but does not save changes to default config
const setRuntimeFillerSettings = async (config:IFillerRuntimeConfig) => {
  const requestData = {
    command: 'SetFillerSettings',
    data: config,
  };

  const apiResult = await webConn?.doPostRequest(requestData);
};

// manual fill x seconds
const startManual = async (fillerId:number, time:number) => {
  const requestData = {
    command: 'StartManual',
    data: {
      id: fillerId,
      time,
    },
  };

  const apiResult = await webConn?.doPostRequest(requestData);
};

onMounted(() => {
  getData();
});

const start = async (id:number) => {
  const requestData = {
    command: 'Start',
    data: {
      id,
    },
  };

  await webConn?.doPostRequest(requestData);
};

</script>

<template>
  <v-container class="pa-6" fluid>
    <div class="d-flex flex-row flex-wrap mb-3">
      <div v-for="fillerConfig of fillerConfigs" :key="fillerConfig.id">
        <FillerControl :fillerConfig="fillerConfig" @start="start" @set="setRuntimeFillerSettings" @startManual="startManual" />
      </div>
    </div>

  </v-container>
</template>
