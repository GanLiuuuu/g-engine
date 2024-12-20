<template>
  <div class="hello">
    <button @click="callBackend">Call Backend</button>
    <p v-if="backendResponse">{{ backendResponse }}</p>
  </div>
</template>

<script>
import axios from 'axios';

export default {
  data() {
    return {
      backendResponse: ''
    };
  },
  methods: {
    callBackend() {
      axios.get('/api/hello')
        .then(response => {
          this.backendResponse = response.data;
        })
        .catch(error => {
          console.error(error);
        });
    }
  }
};
</script>
