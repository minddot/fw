<template>
  <v-app id="inspire">
    <v-app-bar color="green accent-4" dark fixed app clipped-left>
      <v-app-bar-nav-icon @click.stop="drawer = !drawer"></v-app-bar-nav-icon>

      <v-toolbar-title>Mindot Home</v-toolbar-title>
    </v-app-bar>
    <v-navigation-drawer v-model="drawer" fixed app clipped>
      <v-list dense>
        <v-list-item to="/">
          <v-list-item-action>
            <v-icon>home</v-icon>
          </v-list-item-action>
          <v-list-item-content>
            <v-list-item-title>Home</v-list-item-title>
          </v-list-item-content>
        </v-list-item>
        <v-list-item to="/chart">
          <v-list-item-action>
            <v-icon>show_chart</v-icon>
          </v-list-item-action>
          <v-list-item-content>
            <v-list-item-title>Chart</v-list-item-title>
          </v-list-item-content>
        </v-list-item>
        <v-list-item to="/light">
          <v-list-item-action>
            <v-icon>highlight</v-icon>
          </v-list-item-action>
          <v-list-item-content>
            <v-list-item-title>Light</v-list-item-title>
          </v-list-item-content>
        </v-list-item>
      </v-list>
    </v-navigation-drawer>

    <v-main>
      <v-container fluid fill-height>
        <router-view></router-view>
      </v-container>
    </v-main>
    <v-footer color="green accent-4" app fixed>
      <span class="white--text"
        >&copy; SIEMENS T CED INW. All rights reserved.</span
      >
    </v-footer>
  </v-app>
</template>

<script>
export default {
  name: 'App',
  data () {
    return {
      drawer: null
    }
  }
}
</script>
