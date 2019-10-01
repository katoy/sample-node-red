function doGet(e) {
  // データ data1 を取り出す
  var value_1 = e.parameter.data1;
 
  // 現状 Active になっている sheet を取得する
  var sheet = SpreadsheetApp.getActiveSheet();
 
  // 1 列目に日時、2 列目に1、３ 列目に送信されたデータ値を追記する
  sheet.appendRow([new Date(), 1, value_1]);
}
