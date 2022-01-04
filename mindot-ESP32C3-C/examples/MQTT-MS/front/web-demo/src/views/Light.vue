
<template>
  <v-container>
    <v-layout text-xs-center wrap>
      <v-flex xs12 sm6 offset-sm3>
        <v-stepper v-model="e6" vertical>
          <v-stepper-step :complete="e6 > 1" step="1">
            Set WiFi AP
          </v-stepper-step>

          <v-stepper-content step="1">
            <v-text-field
              v-model="ssid"
              :rules="rules"
              counter="25"
              hint="This is station to be connected SSID"
              label="STA SSID"
              clearable
            ></v-text-field>

            <v-text-field
              v-model="passwd"
              :rules="rules"
              counter="25"
              hint="This is station to be connected password"
              label="STA PASSWD"
              clearable
            ></v-text-field>

            <v-text-field
              v-model="ap_ssid"
              :rules="rules"
              counter="25"
              hint="This is AP SSID"
              label="AP SSID"
              clearable
            ></v-text-field>

            <v-text-field
              v-model="ap_passwd"
              :rules="rules"
              counter="25"
              hint="This is AP password"
              label="AP PASSWD"
              clearable
            ></v-text-field>

            <v-btn color="primary" @click="e6 = 2"> Continue </v-btn>
            <v-btn text> Cancel </v-btn>
          </v-stepper-content>

          <v-stepper-step :complete="e6 > 2" step="2">
            Configure MQTT parameters
          </v-stepper-step>

          <v-stepper-content step="2">
            <v-text-field
              v-model="broker"
              hint="Example: avwhozk99tqmx-ats.iot.eu-central-1.amazonaws.com:8883"
              label="Broker URI"
              clearable
              prefix="mqtts://"
            ></v-text-field>
            <v-text-field
              v-model="clientid"
              hint="Format: Tenant_DevName"
              label="Client ID"
              clearable
            ></v-text-field>

            <v-text-field
              v-model="tenant"
              hint="the MindSphere Tenant you are using"
              label="MindSphere Tenant"
              clearable
            ></v-text-field>

            <v-file-input
              accept=".pem"
              label="Amazon Root CA"
              hint="only support PEM format"
              v-model="amazonRootCA"
              @change="upload_root_ca"
            ></v-file-input>

            <v-file-input
              accept=".pem"
              label="Device Certificate"
              hint="only support PEM format"
              v-model="devCert"
              @change="upload_dev_cert"
            ></v-file-input>

            <v-file-input
              accept=".key"
              label="Device Key"
              hint="only support .key format"
              v-model="devKey"
              @change="upload_dev_key"
            ></v-file-input>

            <v-btn color="primary" @click="e6 = 3"> Continue </v-btn>
            <v-btn text @click="e6 = 1"> Cancel </v-btn>
          </v-stepper-content>

          <v-stepper-step :complete="e6 > 3" step="3">
            Input data model
          </v-stepper-step>

          <v-stepper-content step="3">
            <v-file-input
              accept=".json"
              label="Data Model"
              hint="only support JSON format"
              v-model="dataModel"
              @change="upload_data_model"
            ></v-file-input
            ><v-file-input
              accept=".json"
              label="Map Model"
              hint="only support JSON format"
              v-model="mappingModel"
              @change="upload_mapping_model"
            ></v-file-input>
            <v-btn color="primary" @click="e6 = 4"> Continue </v-btn>
            <v-btn text @click="e6 = 2"> Cancel </v-btn>
          </v-stepper-content>

          <v-stepper-step step="4"> Submit all </v-stepper-step>
          <v-stepper-content step="4">
            <v-col align="center" no-gutters>
              <v-btn
                fab
                dark
                large
                color="red accent-4"
                @click="submit_paramter"
              >
                <v-icon dark>check_box</v-icon>
              </v-btn>
            </v-col>
            <v-col>
              <v-row>
                <v-btn color="primary" @click="e6 = 1"> Continue </v-btn>
                <v-btn text @click="e6 = 3"> Cancel </v-btn></v-row
              >
            </v-col>
          </v-stepper-content>
        </v-stepper>
      </v-flex>
    </v-layout>
  </v-container>
</template>

<script>
export default {
  data () {
    return {
      ssid: null,
      passwd: null,
      ap_ssid: null,
      ap_passwd: null,
      broker: null,
      clientid: null,
      tenant: null,
      e6: 1,
      amazonRootCA: null,
      devCert: null,
      devKey: null,
      dataModel: null,
      mappingModel: null,
      rules: [(v) => (v && v.length <= 25) || !v || 'Max 25 characters']
    }
  },
  methods: {
    upload_file (data, url) {
      return this.$ajax.post(url, {
        context: data
      })
    },
    submit_paramter () {
      var param = {}

      if (this.ssid != null) {
        param['SSID'] = this.ssid
      }
      if (this.passwd != null) {
        param['PASSWD'] = this.passwd
      }
      if (this.ap_ssid != null) {
        param['AP_SSID'] = this.ap_ssid
      }
      if (this.ap_passwd != null) {
        param['AP_PASSWD'] = this.ap_passwd
      }
      if (this.broker != null) {
        param['Broker'] = 'mqtts://' + this.broker
      }
      if (this.clientid != null) {
        param['Client_Id'] = this.clientid
      }
      if (this.tenant != null) {
        param['Tenant'] = this.tenant
      }

      this.$ajax
        .post('/api/v1/config/param', param)
    },
    upload_root_ca () {
      let reader = new FileReader()
      reader.readAsText(this.amazonRootCA)
      reader.onload = () => {
        this.upload_file(reader.result, '/api/v1/upload/amazon-CA')
      }
    },
    upload_dev_cert () {
      let reader = new FileReader()
      reader.readAsText(this.devCert)
      reader.onload = () => {
        this.upload_file(reader.result, '/api/v1/upload/dev-cert')
      }
    },
    upload_dev_key () {
      let reader = new FileReader()
      reader.readAsText(this.devKey)
      reader.onload = () => {
        this.upload_file(reader.result, '/api/v1/upload/dev-key')
      }
    },
    upload_data_model () {
      let reader = new FileReader()
      reader.readAsText(this.dataModel)
      reader.onload = () => {
        this.upload_file(reader.result, '/api/v1/upload/data-model')
      }
    },
    upload_mapping_model () {
      let reader = new FileReader()
      reader.readAsText(this.mappingModel)
      reader.onload = () => {
        this.upload_file(reader.result, '/api/v1/upload/mapping-model')
      }
    }

  }
}
</script>
