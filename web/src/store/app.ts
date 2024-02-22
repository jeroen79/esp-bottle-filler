/* eslint-disable import/prefer-default-export */
/* eslint-disable import/order */
import { defineStore } from 'pinia';
import { ref } from 'vue';

export const useAppStore = () => {
  const innerStore = defineStore('app', () => {
    const systemSettingsLoaded = ref(false);
    const rootUrl = ref<string | null>(null);

    async function getSystemSettings() {
      // no settings atm, will propbalby be needed in the future
      // if (rootUrl.value == null) {
      //   return;
      // }

      //   // const webConn = new WebConn(rootUrl.value);
      //   // const requestData = {
      //   //   command: 'GetSystemSettings',
      //   //   data: null,
      //   // };

      //   // const apiResult = await webConn?.doPostRequest(requestData);

      //   // if (apiResult === undefined || apiResult.success === false) {
      //   //   return;
      //   // }

      systemSettingsLoaded.value = true;
      return {
      };
    }

    return {
      systemSettingsLoaded, rootUrl, getSystemSettings,
    };
  });

  const s = innerStore();
  // load system settings when possible
  // if (s.rootUrl !== null && !s.systemSettingsLoaded) {
  //   s.getSystemSettings();
  // }
  return s;
};
