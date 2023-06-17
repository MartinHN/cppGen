set -e

ts-node ../../../run.ts uapi.json

echo "start JS Test"
npm run test
