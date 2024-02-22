// Composables
import { createRouter, createWebHistory } from 'vue-router';

const routes = [
  {
    path: '/',
    component: () => import('@/layouts/default/View.vue'),
    children: [
      {
        path: '',
        name: 'Home',
        // route level code-splitting
        // this generates a separate chunk (about.[hash].js) for this route
        // which is lazy-loaded when the route is visited.
        component: () => import(/* webpackChunkName: "home" */ '@/views/Control.vue'),
      },
      {
        path: 'control',
        name: 'Control',
        component: () => import(/* webpackChunkName: "Control" */ '@/views/Control.vue'),
      },
      {
        path: 'wifiSettings',
        name: 'WifiSettings',
        component: () => import(/* webpackChunkName: "WifiSettings" */ '@/views/WifiSettings.vue'),
      },
      {
        path: 'fillerSettings',
        name: 'FillerSettings',
        component: () => import(/* webpackChunkName: "FillerSettings" */ '@/views/FillerSettings.vue'),
      },
      {
        path: 'systemSettings',
        name: 'SystemSettings',
        component: () => import(/* webpackChunkName: "SystemSettings" */ '@/views/SystemSettings.vue'),
      },
    ],
  },
];

const router = createRouter({
  history: createWebHistory(process.env.BASE_URL),
  routes,
});

export default router;
