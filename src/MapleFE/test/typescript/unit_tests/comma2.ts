var arr: number[] = [-1, +4, 5, 9, 2, 8, 1, 6, 3];
var sum: number = 0;
var i;
var len;
i = 0, len = arr.length;
for (; i < len; ++i) {
  sum += arr[i];
}
console.log(sum);
