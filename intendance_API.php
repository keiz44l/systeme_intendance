function doGet(e) { 
  Logger.log( JSON.stringify(e) ); 

  var result = 'Ok';

  if (e.parameter == undefined) {
    result = 'No Parameters';
  }
  else {
    var id = '1P9I598da6CKS4ZkwH43D1U1UqzM0N0cJCGlAtM7aGjI'; // google sheet ID
    var sheet = SpreadsheetApp.openById(id).getActiveSheet();
    var newRow = sheet.getLastRow() + 1;
    var fingerprintData = []; // tableau contenant l'heure, l'existence et l'id
    fingerprintData[0] = new Date(); //initialise l'heure 
    for (var param in e.parameter) {
      Logger.log('In for loop, param='+param);
      var value = stripQuotes(e.parameter[param]);
      switch (param) {
        case 'allowed_members': //Paramètre sur l'existence ou nom de l'élève
          fingerprintData[1] = value; //valeur se met dans la colonne B
          break;
        case 'Member_ID': //identifiant unique de l'élève
          fingerprintData[2] = value; //valeur se met dans la colonne C
          break;
        default:
          result = "Wrong parameter";  
      }
    }
    Logger.log(JSON.stringify(fingerprintData));

    // cré une nouvelle rangée en bas
    var newRange = sheet.getRange(newRow, 1, 1, fingerprintData.length);
    newRange.setValues([fingerprintData]);
  }

  // retourne le résultat de l'opération
  return ContentService.createTextOutput(result);
}

/**
* retire les guillemets
*/
function stripQuotes( value ) {
  return value.replace(/^["']|['"]$/g, "");
}
