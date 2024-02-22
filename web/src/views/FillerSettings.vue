<script lang="ts" setup>
import { mdiDelete, mdiPencil } from '@mdi/js';
import { inject, onBeforeUnmount, onMounted, ref } from 'vue';
import { VDataTable } from 'vuetify/labs/VDataTable';
import { IFillerConfig } from '@/interfaces/IFillerConfig';
import WebConn from '@/helpers/webConn';

const webConn = inject<WebConn>('webConn');

const fillerConfigs = ref<Array<IFillerConfig>>([]);

const tableHeaders = ref<Array<any>>([
  { title: 'Id', key: 'id', align: 'start' },
  { title: 'Name', key: 'name', align: 'start' },
  { title: 'Pump Pin', key: 'pumpPin', align: 'start' },
  { title: 'Auto Button Pin', key: 'autoPin', align: 'start' },
  { title: 'Auto Speed (%)', key: 'autoFillSpeed', align: 'end' },
  { title: 'Auto Time (ms)', key: 'fillTime', align: 'end' },
  { title: 'Manual Speed (%)', key: 'manualFillSpeed', align: 'end' },
  { title: 'Manual Button Pin', key: 'manualPin', align: 'start' },
  { title: 'Actions', key: 'actions', align: 'end', sortable: false },
]);

const dialog = ref<boolean>(false);
const dialogDelete = ref<boolean>(false);

const alert = ref<string>('');
const alertType = ref<'error' | 'success' | 'warning' | 'info' >('info');

const defaultFiller:IFillerConfig = {
  id: 0,
  name: 'New Filler',
  pumpPin: 0,
  autoPin: 0,
  manualPin: 0,
  autoFillSpeed: 0,
  manualFillSpeed: 0,
  fillTime: 0,
};

const editedItem = ref<IFillerConfig>(defaultFiller);

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

onMounted(() => {
  getData();
});

onBeforeUnmount(() => {

});

const closeDialog = async () => {
  dialog.value = false;
};

const closeDeleteDialog = async () => {
  dialogDelete.value = false;
};

const editItem = async (item:IFillerConfig) => {
  editedItem.value = item;
  dialog.value = true;
};

const openDeleteDialog = async (item:IFillerConfig) => {
  editedItem.value = item;
  dialogDelete.value = true;
};

const deleteItemOk = async () => {
  // filter out deleted
  fillerConfigs.value = fillerConfigs.value.filter((s) => s.id !== editedItem.value.id);
  closeDeleteDialog();
};

const save = async () => {
  if (fillerConfigs.value == null) {
    return;
  }

  // alert.value = 'Io changes require a reboot!';
  // alertType.value = 'info';

  const requestData = {
    command: 'SaveFillerSettings',
    data: fillerConfigs.value,
  };

  await webConn?.doPostRequest(requestData);
  // todo capture result and log errors
};

</script>

<template>
  <v-container class="pa-6" fluid>
    <v-alert :type="alertType" v-if="alert">{{alert}}</v-alert>
    <v-form fast-fail @submit.prevent>
      <v-data-table
        :headers="tableHeaders"
        :items="fillerConfigs"
        density="compact"
        item-value="name"
      >
        <template v-slot:top>
          <v-toolbar density="compact">
            <v-toolbar-title>Filler Configurations</v-toolbar-title>
            <v-spacer />
            <v-btn color="secondary" variant="outlined" class="mr-5" @click="getData()">
              Refresh
            </v-btn>

            <v-dialog v-model="dialog" max-width="500px">
              <v-card>
                <v-card-title>
                  <span class="text-h5">Edit</span>
                </v-card-title>

                <v-card-text>
                  <v-container>
                    <v-row>
                      <v-text-field v-model="editedItem.name" label="Name" />
                    </v-row>
                    <v-row>
                      <v-text-field v-model.number="editedItem.pumpPin" label="Pump Pin" />
                    </v-row>
                    <v-row>
                      <v-text-field v-model.number="editedItem.autoPin" label="Auto Button Pin" />
                    </v-row>
                    <v-row>
                      <v-text-field v-model.number="editedItem.autoFillSpeed" label="Auto Speed (%)" />
                    </v-row>
                    <v-row>
                      <v-text-field v-model.number="editedItem.fillTime" label="Auto Time (ms)" />
                    </v-row>
                    <v-row>
                      <v-text-field v-model.number="editedItem.manualPin" label="Manual Button Pin" />
                    </v-row>
                    <v-row>
                      <v-text-field v-model.number="editedItem.manualFillSpeed" label="Manual Speed (%)" />
                    </v-row>
                  </v-container>
                </v-card-text>

                <v-card-actions>
                  <v-spacer />
                  <v-btn color="blue-darken-1" variant="text" @click="closeDialog">
                    Close
                  </v-btn>
                </v-card-actions>
              </v-card>
            </v-dialog>
            <v-dialog v-model="dialogDelete" max-width="500px">
              <v-card>
                <v-card-title class="text-h5">Are you sure you want to delete this item?</v-card-title>
                <v-card-actions>
                  <v-spacer />
                  <v-btn color="blue-darken-1" variant="text" @click="closeDeleteDialog">Cancel</v-btn>
                  <v-btn color="blue-darken-1" variant="text" @click="deleteItemOk">OK</v-btn>
                  <v-spacer />
                </v-card-actions>
              </v-card>
            </v-dialog>
          </v-toolbar>
        </template>
        <template v-slot:[`item.actions`]="{ item }">
          <v-icon size="small" class="me-2" @click="editItem(item.raw)" :icon="mdiPencil" />
          <v-icon size="small" @click="openDeleteDialog(item.raw)" :icon="mdiDelete" />
        </template>
      </v-data-table>

      <v-btn color="success" class="mt-4 mr-2" @click="save"> Save </v-btn>

    </v-form>
  </v-container>
</template>
