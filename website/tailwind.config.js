/** @type {import('tailwindcss').Config} */
export default {
  darkMode: 'class',
  content: ['./index.html', './src/**/*.{js,jsx,ts,tsx}'],
  theme: {
	extend: {
	  colors: {
		vcam: {
		  teal: '#0f766e',
		  mint: '#d1fae5',
		  coral: '#fb7185',
		},
	  },
	},
  },
  plugins: [],
}

