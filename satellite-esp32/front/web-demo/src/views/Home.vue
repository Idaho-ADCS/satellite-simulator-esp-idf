<template>
  <v-container>
    <v-layout text-xs-center wrap>
      <v-flex xs12 sm4 offset-sm4>
        <v-switch
          v-model="enable"
          color="yellow accent-4"
          inset
          label="System enable"
          @change="set_enable"
        ></v-switch>

        <v-select
          v-model="mode"
          color="yellow accent-4"
          :items="modes"
          label="Command"
          :disabled="!enable"
          @change="set_mode"
        ></v-select>
      </v-flex>
    </v-layout>

    <v-divider></v-divider>

    <v-layout row wrap>
      <v-flex xs12 sm12>
        <v-data-table
          :headers="headers"
          :items="packets"
          :items-per-page="10"
          class="elevation-1"
          must-sort
        >
          <template v-slot:items="props">
            <td>{{ props.item.seq }}</td>
            <td>{{ props.item.status }}</td>
            <td>{{ props.item.voltage }}</td>
            <td>{{ props.item.current }}</td>
            <td>{{ props.item.speed }}</td>
            <td>{{ props.item.magx }}</td>
            <td>{{ props.item.magy }}</td>
            <td>{{ props.item.magz }}</td>
            <td>{{ props.item.gyrox }}</td>
            <td>{{ props.item.gyroy }}</td>
            <td>{{ props.item.gyroz }}</td>
          </template>
        </v-data-table>
      </v-flex>
    </v-layout>
  </v-container>
</template>

<script>
export default {
  data() {
    return {
      timer: null,
      enable: false,
      mode: "Standby",
      modes: ["Standby", "Measure", "Simple Detumble Test"],

      headers: [
        {
          text: "Sequence",
          value: "seq",
        },
        {
          text: "Status",
          value: "status",
          sortable: false,
        },
        {
          text: "Voltage (V)",
          value: "voltage",
          sortable: false,
        },
        {
          text: "Current (mA)",
          value: "current",
          sortable: false,
        },
        {
          text: 'Motor Speed (RPS)',
          value: 'speed',
          sortable: false
        },
        {
          text: "Mag X (uT)",
          value: "magx",
          sortable: false,
        },
        {
          text: "Mag Y (uT)",
          value: "magy",
          sortable: false,
        },
        {
          text: "Mag Z (uT)",
          value: "magz",
          sortable: false,
        },
        {
          text: "Gyro X (deg/s)",
          value: "gyrox",
          sortable: false,
        },
        {
          text: "Gyro Y (deg/s)",
          value: "gyroy",
          sortable: false,
        },
        {
          text: "Gyro Z (deg/s)",
          value: "gyroz",
          sortable: false,
        },
      ],

      packets: [],
    };
  },

  methods: {
    set_enable: function () {
      if (!this.enable) {
        this.mode = "Standby";
        clearInterval(this.timer);
      }

      this.$ajax
        .post("/api/adcs/enable", {
          enable: this.enable,
        })
        .then((res) => {
          console.log(res);
        })
        .catch((err) => {
          console.log(err);
        });
    },

    set_mode: function () {
      let modeInt;

      if (this.mode === "Standby") {
        modeInt = 0;
        clearInterval(this.timer);
      }
      if (this.mode === "Measure") {
        modeInt = 1;
        clearInterval(this.timer);
        this.timer = setInterval(this.updateData, 550);
      }
	  if (this.mode === "Simple Detumble Test") {
		modeInt = 2;
		clearInterval(this.timer);
	  }

      this.$ajax
        .post("/api/adcs/mode", {
          mode: modeInt,
        })
        .then((res) => {
          console.log(res);
        })
        .catch((err) => {
          console.log(err);
        });
    },

    updateData: function () {
      this.$ajax
        .get("/api/adcs/data")
        .then((res) => {
          this.packets.push(res.data);
        })
        .catch((err) => {
          console.log(err);
        });
    },
  },

  mounted() {
    clearInterval(this.timer);
  },

  destroyed: function () {
    clearInterval(this.timer);
  },
};
</script>
