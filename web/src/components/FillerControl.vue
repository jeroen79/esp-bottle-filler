<script lang="ts" setup>
import { mdiPlus, mdiMinus } from '@mdi/js';
import { ref, watch } from 'vue';
import debounce from 'lodash.debounce';
import { IFillerConfig } from '@/interfaces/IFillerConfig';
import { IFillerRuntimeConfig } from '@/interfaces/IFillerRuntimeConfig';

const props = defineProps<{
  fillerConfig: IFillerConfig
}>();

// we do not want reactivity here so we can ignore no-setup-props-destructure
// eslint-disable-next-line vue/no-setup-props-destructure
const autoFillSpeed = ref(props.fillerConfig.autoFillSpeed);
// eslint-disable-next-line vue/no-setup-props-destructure
const manualFillSpeed = ref(props.fillerConfig.manualFillSpeed);
// eslint-disable-next-line vue/no-setup-props-destructure
const fillTime = ref(props.fillerConfig.fillTime);

const emit = defineEmits<{
  (e: 'start', id: number): void
  (e: 'set', config:IFillerRuntimeConfig): void
  (e: 'startManual', id: number, time:number): void
}>();

const start = async () => {
  emit('start', props.fillerConfig.id);
};

const incrementAutoSpeed = async () => {
  if (autoFillSpeed.value < 100) {
    autoFillSpeed.value += 1;
  }
};

const decrementAutoSpeed = async () => {
  if (autoFillSpeed.value > 0) {
    autoFillSpeed.value -= 1;
  }
};

const incrementTime = async () => {
  if (fillTime.value < 50000) {
    fillTime.value += 100;
  }
};

const decrementTime = async () => {
  if (fillTime.value > 100) {
    fillTime.value -= 100;
  }
};

const incrementManualSpeed = async () => {
  if (manualFillSpeed.value < 100) {
    manualFillSpeed.value += 1;
  }
};

const decrementManualSpeed = async () => {
  if (manualFillSpeed.value > 0) {
    manualFillSpeed.value -= 1;
  }
};

const setValues = async () => {
  const newRuntimeValues:IFillerRuntimeConfig = {
    id: props.fillerConfig.id,
    autoFillSpeed: autoFillSpeed.value,
    manualFillSpeed: manualFillSpeed.value,
    fillTime: fillTime.value,
  };
  emit('set', newRuntimeValues);
};

const manualFill = async (time:number) => {
  emit('startManual', props.fillerConfig.id, time);
};

const debounceSet = debounce(setValues, 1000);

watch(() => manualFillSpeed.value, debounceSet);
watch(() => fillTime.value, debounceSet);
watch(() => autoFillSpeed.value, debounceSet);

</script>

<template>
  <v-container class="ma-2 pa-2">
    <v-card width="300">
      <v-card-title class="bg-primary">
        <span class="text-h5">{{props.fillerConfig.name}}</span>
      </v-card-title>

      <v-card-text>
        <v-row>
          <v-col>
            <div class="text-caption mt-2">
              Auto Fill Speed:
            </div>
            <div class="text-h5 font-weight-light text-center">{{ autoFillSpeed }} %</div>
            <v-slider hide-details v-model="autoFillSpeed" color="green" track-color="grey" min="1" max="100" step="1">
              <template v-slot:prepend>
                <v-btn size="small" variant="text" @click="decrementAutoSpeed" :icon="mdiMinus" />
              </template>
              <template v-slot:append>
                <v-btn size="small" variant="text" @click="incrementAutoSpeed" :icon="mdiPlus" />
              </template>
            </v-slider>
          </v-col>
        </v-row>

        <v-row class="mb-0 mt-0">
          <v-col>
            <div class="text-caption">
              Auto Fill Time:
            </div>
            <div class="text-h5 font-weight-light text-center">{{ fillTime }} ms</div>
            <v-slider hide-details v-model="fillTime" color="red" track-color="grey" min="100" max="50000" step="500">
              <template v-slot:prepend>
                <v-btn size="small" variant="text" @click="decrementTime" :icon="mdiMinus" />
              </template>
              <template v-slot:append>
                <v-btn size="small" variant="text" @click="incrementTime" :icon="mdiPlus" />
              </template>
            </v-slider>
          </v-col>
        </v-row>

        <v-row class="mb-0 mt-0">
          <v-col>
            <v-btn color="success" class="mt-0" block @click="start"> Auto Fill Start </v-btn>
          </v-col>
        </v-row>
        <v-divider />
        <v-row class="mb-0 mt-0">
          <v-col>
            <div class="text-caption">
              Manual Fill Speed:
            </div>
            <div class="text-h5 font-weight-light text-center">{{ manualFillSpeed }} %</div>
            <v-slider hide-details v-model="manualFillSpeed" color="orange" track-color="grey" min="1" max="100" step="1">
              <template v-slot:prepend>
                <v-btn size="small" variant="text" @click="decrementManualSpeed" :icon="mdiMinus" />
              </template>
              <template v-slot:append>
                <v-btn size="small" variant="text" @click="incrementManualSpeed" :icon="mdiPlus" />
              </template>
            </v-slider>
          </v-col>
        </v-row>

        <v-row class="mb-0 mt-0">
          <v-col>
            <div class="text-caption mb-1">
              Manual Fill (s):
            </div>
            <div class="text-center">
              <v-btn color="orange" rounded variant="outlined" @click="manualFill(500)" class="mr-1 font-weight-light">0.5</v-btn>
              <v-btn color="orange" rounded variant="outlined" @click="manualFill(1000)" class="mr-1 font-weight-light">1</v-btn>
              <v-btn color="orange" rounded variant="outlined" @click="manualFill(1500)" class="mr-1 font-weight-light">1.5</v-btn>
              <v-btn color="orange" rounded variant="outlined" @click="manualFill(2000)" class="font-weight-light">2</v-btn>
            </div>

          </v-col>
        </v-row>

      </v-card-text>
    </v-card>
  </v-container>

</template>
