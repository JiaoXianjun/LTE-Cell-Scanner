//------------------------------------------------------------------------------
// file properties
//------------------------------------------------------------------------------
//  file     : smta.js
//  date     : 16-09-2017
//  version  : 2.0
//  language : Javascript, CSS
//  author   : SMTA developer
//  company  : Spectrum Monitoring Technology Advisors bv
//------------------------------------------------------------------------------


function switchTab(event, tabid)
{
  var i, tabcontent;
  tabcontent = document.getElementsByName(event.currentTarget.name.replace("button","content"));
  for (i=0; i<tabcontent.length; i++) if (tabid==i) tabcontent[i].style.display = "block"; else tabcontent[i].style.display = "none";
  tablinks = document.getElementsByClassName("smtatabbuttonactive");
  for (i=0; i<tablinks.length; i++) { if (tablinks[i].name == event.currentTarget.name) tablinks[i].className = tablinks[i].className.replace("active", ""); }
  event.currentTarget.className += "active";
}


function switchDisplayMode(event, mode)
{
  marketheader = document.getElementsByName("marketheader");
  operatorlegend = document.getElementsByName("operatorlegend");
  if ((marketheader.length > 0) && (operatorlegend.length > 0))
  {
    if (marketheader[0].style.display == "none")
    {
      for (i=0; i<marketheader.length; i++) marketheader[i].style.display = "block";
      for (i=0; i<operatorlegend.length; i++) operatorlegend[i].style.display = "block";
    }
    else
    {
      for (i=0; i<marketheader.length; i++) marketheader[i].style.display = "none";
      for (i=0; i<operatorlegend.length; i++) operatorlegend[i].style.display = "none";
    }
  }
}


function showChartSettings(event, market)
{
  countrysettingsdiv = document.getElementById(market+'countrysettings');
  areasettingsdiv = document.getElementById(market+'areasettings');
  viewsettingsdiv = document.getElementById(market+'viewsettings');
  advertisement2div = document.getElementById(market+'advertisement');
  if (areasettingsdiv.style.display == 'block') displayval = 'none'; else displayval = 'block';
  countrysettingsdiv.style.display = displayval;
  areasettingsdiv.style.display = displayval;
  viewsettingsdiv.style.display = displayval;
  advertisement2div.style.display = displayval;
  return 0;
}


function showSpectrumChart(event)
{
  var location = 'frequencies.php';
  if (document.getElementById('selectbyband').style.display == 'block')
  {
    var firstbandchecked = false;
    var checkboxes = document.getElementsByName("bandselectbox");
    for (i=0; i<checkboxes.length; i++) if (checkboxes[i].checked) { if (!firstbandchecked) { firstbandchecked=true; location +='?band=' } else { location += '+' } location += checkboxes[i].id; }
    var firstmarketchecked = false;
    var checkboxes = document.getElementsByName("marketselectbox");
    for (i=0; i<checkboxes.length; i++) if (checkboxes[i].checked) { if (!firstmarketchecked) { firstmarketchecked=true; firstbandchecked==true?location += '&':location +='?'; location += 'market=' } else { location += '+' } location += checkboxes[i].id; }

  }
  else if (document.getElementById('selectbycarrier').style.display == 'block')
  {
    var firstcarrierchecked = false;
    var checkboxes = document.getElementsByName("carrierselectbox");
    for (i=0; i<checkboxes.length; i++) if (checkboxes[i].checked) { if (!firstcarrierchecked) { firstcarrierchecked=true; location +='?carrier=' } else { location += '+' } location += checkboxes[i].id; }
  }
  else if (document.getElementById('selectbymarket').style.display == 'block')
  {
    var firstmarketchecked = false;
    var checkboxes = document.getElementsByName("marketselectbox");
    for (i=0; i<checkboxes.length; i++) if (checkboxes[i].checked) { if (!firstmarketchecked) { firstmarketchecked=true; location +='?market=' } else { location += '+' } location += checkboxes[i].id; }
  }
  window.location=location;
}


function calculateFrequency(event)
{
  var location = 'systems.php?channel=' + Math.round(document.getElementById("calculatorselectbox").value);
  window.location=location;
}


function populateselectcountry(selectcountryname,selectedcountrycode)
{
  var selectcountry = document.getElementById(selectcountryname);
  for(i=0; i<C.length; i++)
  {
    selectcountry.options[selectcountry.options.length] = new Option(C[i].length>4?C[i][1]+" ->":C[i][1], C[i][0]);
    for (j=0; j<C[i].length; j+=2) if (C[i][j] == selectedcountrycode) selectcountry.selectedIndex = i+1;
  }
}


function populateselectmarket(selectcountryname, selectmarketname, selectedmarketcode)
{
  var selectcountry = document.getElementById(selectcountryname);
  var selectmarket = document.getElementById(selectmarketname);
  if (selectedmarketcode.length == 0) selectedmarketcode = selectcountry.value; 
  selectmarket.options.length = 0;
  for(i=0; i<C.length; i++)
  {
    if (C[i][0] == selectcountry.value)
    {
      selectmarket.options[selectmarket.options.length] = new Option("- All areas -", "ALL");
      for(j=2; j<C[i].length; j+=2) { selectmarket.options[selectmarket.options.length] = new Option(C[i][j+1], C[i][j]); if (C[i][j]==selectedmarketcode) selectmarket.selectedIndex = selectmarket.options.length-1; }
    }
  }
}


function populateselectband(selectbandname)
{
  var selectband = document.getElementById(selectbandname);
  if (B.length > 1) selectband.options[selectband.options.length] = new Option("- All bands -", "ALL");
  for(i=0; i<B.length; i++) selectband.options[selectband.options.length] = new Option(B[i], B[i]);
}


function populateselectgroup(selectgroupname,selectedgroup)
{
  var selectgroup = document.getElementById(selectgroupname);
  for(i=0; i<G.length; i++)
  {
    selectgroup.options[selectgroup.options.length] = new Option(G[i], G[i]);
    if (G[i] == selectedgroup) selectgroup.selectedIndex = i+1;
  }
}


function populateselectoperator(selectoperatorname,selectedoperator)
{
  var selectoperator = document.getElementById(selectoperatorname);
  for(i=0; i<O.length; i++) 
  {
    selectoperator.options[selectoperator.options.length] = new Option(O[i], O[i]);
    if (O[i] == selectedoperator) selectoperator.selectedIndex = i+1;
  }
}


function addgeofilter(divfiltername,selectcountryname,selectmarketname,buttonremovegeofiltername,buttonaddgeofiltername,remove)
{
  var divfilter = document.getElementById(divfiltername);
  if (remove) divfilter.style.height = parseint(divfilter.style.height) - 20 + "px"; else divfilter.style.height = parseInt(divfilter.style.height) + 20 + "px";
  var selectcountry = document.getElementById(selectcountryname);
  if (remove) selectcountry.style.display='none'; else selectcountry.style.display='inline';
  var selectmarket = document.getElementById(selectmarketname);
  if (remove) selectmarket.style.display='none'; else selectmarket.style.display='inline';
  var buttonremovegeofilter = document.getElementById(buttonremovegeofiltername);
  if (remove) buttonremovegeofilter.style.display='none'; else buttonremovegeofilter.style.display='inline';
  var buttonaddgeofilter = document.getElementById(buttonaddgeofiltername);
  if (remove) buttonaddgeofilter.style.display='none'; else buttonaddgeofilter.style.display='inline';
}


function showlicence(selectcountryname, selectmarketname, selectbandname, checkboxlegendname, rangezoomname)
{
  var location = 'licence.php?';
  if ((selectcountryname != null) && (selectmarketname != null)) 
  {
    var selectcountry = document.getElementById(selectcountryname);
    var selectmarket = document.getElementById(selectmarketname);
    if (selectmarket.value=='ALL') location+= "c=" + selectcountry.value;
                              else location+= "m=" + selectmarket.value;
  }
  if (selectbandname != null)
  {
    var selectband = document.getElementById(selectbandname);
    if (selectband.selectedIndex > 0) location+= '&b=' + selectband.value;
  }
  if (checkboxlegendname != null)
  {
    var checkboxlegend = document.getElementById(checkboxlegendname);
    if (checkboxlegend.checked) location += '&l=1';
  }
  if (rangezoomname != null)
  {
    var rangezoom = document.getElementById(rangezoomname);
    if ((rangezoom.value >= 3) && (rangezoom.value <= 7)) location += '&z=' + rangezoom.value;
  }
  window.location=location;
}


function showmarket(selectcountryname, selectmarketname)
{
  var location = 'market.php?';
  var selectcountry = document.getElementById(selectcountryname);
  var selectmarket = document.getElementById(selectmarketname);
  if (selectmarket.value=='ALL') location+= "c=" + selectcountry.value;
                            else location+= "m=" + selectmarket.value;
  window.location=location;
}


function showoperator(selectoperatorname, local)
{
  var selectoperator = document.getElementById(selectoperatorname);
  var location = 'operator.php?';
  if (local==true) location+= "o="; else location+= "g=";
  location += selectoperator.value;
  window.location=location;
}


function rotatebanner(fad1, fad2, fad3)
{
  var adimgs = ["images/adsul.jpg", "images/adbil.jpg", "images/adedu.jpg"];
  var adlinks = ["offers.php", "offers.php", "training.php"];
  var adshow = [fad1?1:0, fad2?1:0, fad3?1:0];
  var adindex = adimgs.indexOf(document.getElementById("adimg").getAttribute("src"));
  var watchdog = 0;
  do
  {
    if (adindex < 0) adindex = 0; else adindex++;
    if (adindex >= adimgs.length) adindex = 0;
    watchdog++;
  }
  while ((!adshow[adindex]) && (watchdog < adimgs.length))
  document.getElementById("adlink").setAttribute("href", adlinks[adindex]);
  document.getElementById("adimg").setAttribute("src", adimgs[adindex]);
}
