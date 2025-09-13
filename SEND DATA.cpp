// --- Config: ganti sesuai milik Anda ---
const SPREADSHEET_ID = "1E2p50oXsjCQR1LrnPC-Qsi_3gPG-xK6KZrfuDpffa8I";
const SECRET_TOKEN = "12345";
// ---------------------------------------

function doPost(e) {
  try {
    // parsing payload (mendukung JSON atau form-data)
    var payload = {};
    if (e.postData && e.postData.contents) {
      payload = JSON.parse(e.postData.contents);
    } else {
      payload.uid = e.parameter.uid;
      payload.token = e.parameter.token;
    }

    if (!payload.token || payload.token !== SECRET_TOKEN) {
      return ContentService
        .createTextOutput(JSON.stringify({ success: false, error: "invalid_token" }))
        .setMimeType(ContentService.MimeType.JSON);
    }
    var uid = String(payload.uid || "").trim();
    if (!uid) {
      return ContentService
        .createTextOutput(JSON.stringify({ success: false, error: "missing_uid" }))
        .setMimeType(ContentService.MimeType.JSON);
    }
    uid = uid.toUpperCase();

    var ss = SpreadsheetApp.openById(SPREADSHEET_ID);
    var membersSheet = ss.getSheetByName("Members");
    var attendSheet = ss.getSheetByName("Attendance");
    if (!membersSheet || !attendSheet) {
      return ContentService
        .createTextOutput(JSON.stringify({ success: false, error: "missing_sheets" }))
        .setMimeType(ContentService.MimeType.JSON);
    }

    // Cari nama & status di sheet Members
    var membersVals = membersSheet.getDataRange().getValues();
    var name = "[Unknown]";
    var status = "[Unknown]";
    for (var i = 1; i < membersVals.length; i++) {
      if (String(membersVals[i][0]).trim().toUpperCase() === uid) {
        name = membersVals[i][1];
        status = membersVals[i][2];
        break;
      }
    }

    // Dapatkan index kolom di Attendance berdasarkan header (1-based)
    var headers = attendSheet.getRange(1, 1, 1, attendSheet.getLastColumn()).getValues()[0];
    var idxWaktuMasuk = headers.indexOf("Waktu Masuk") + 1;
    var idxNama = headers.indexOf("Nama") + 1;
    var idxStatus = headers.indexOf("Status Member") + 1;
    var idxWaktuKeluar = headers.indexOf("Waktu Keluar") + 1;
    var idxUID = headers.indexOf("UID") + 1;
    if ([idxWaktuMasuk, idxNama, idxStatus, idxWaktuKeluar, idxUID].indexOf(0) !== -1) {
      return ContentService
        .createTextOutput(JSON.stringify({ success: false, error: "attendance_headers_missing" }))
        .setMimeType(ContentService.MimeType.JSON);
    }

    // Cari baris terakhir untuk UID ini yang Waktu Keluar kosong (search dari bawah)
    var lastRow = Math.max(attendSheet.getLastRow(), 1);
    var foundRow = null;
    if (lastRow >= 2) {
      var dataBlock = attendSheet.getRange(2, 1, lastRow - 1, attendSheet.getLastColumn()).getValues();
      for (var r = dataBlock.length - 1; r >= 0; r--) {
        var rowUID = String(dataBlock[r][idxUID - 1]).trim().toUpperCase();
        var rowWktKeluar = dataBlock[r][idxWaktuKeluar - 1];
        if (rowUID === uid && (!rowWktKeluar || rowWktKeluar === "")) {
          foundRow = r + 2; // karena dataBlock dimulai dari baris 2
          break;
        }
      }
    }

    var now = new Date();
    if (foundRow) {
      // checkout: isi Waktu Keluar pada row yang ditemukan
      attendSheet.getRange(foundRow, idxWaktuKeluar).setValue(now);
      return ContentService
        .createTextOutput(JSON.stringify({ success: true, action: "checkout", name: name, status: status, uid: uid }))
        .setMimeType(ContentService.MimeType.JSON);
    } else {
      // checkin: tambahkan baris baru
      var colCount = attendSheet.getLastColumn();
      var newRow = new Array(colCount).fill("");
      newRow[idxWaktuMasuk - 1] = now;
      newRow[idxNama - 1] = name;
      newRow[idxStatus - 1] = status;
      newRow[idxWaktuKeluar - 1] = "";
      newRow[idxUID - 1] = uid;
      attendSheet.appendRow(newRow);
      return ContentService
        .createTextOutput(JSON.stringify({ success: true, action: "checkin", name: name, status: status, uid: uid }))
        .setMimeType(ContentService.MimeType.JSON);
    }
  } catch (err) {
    return ContentService
      .createTextOutput(JSON.stringify({ success: false, error: err.toString() }))
      .setMimeType(ContentService.MimeType.JSON);
  }
}