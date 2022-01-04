import Vue from 'vue'
// import './plugins/vuetify'
import vuetify from './plugins/vuetify'
import App from './App.vue'
import router from './router'
import axios from 'axios'
import store from './store'

Vue.config.productionTip = false

Vue.prototype.$ajax = axios

new Vue({
  router,
  store,
  vuetify,
  render: h => h(App)
}).$mount('#app')
